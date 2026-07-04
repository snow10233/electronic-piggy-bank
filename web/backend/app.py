from __future__ import annotations

from threading import Lock

import requests
from flask import Flask, jsonify, request
from flask_cors import CORS
from mysql.connector import Error as MySQLError

from .config import AppConfig
from .database import Database, quote_identifier, rows_to_money_records
from .face_service import FaceRecognitionService
from .hardware import SerialClient


SORT_OPTIONS = {
    "time": "ORDER BY `time`",
    "time-desc": "ORDER BY `time` DESC",
    "money": "ORDER BY `money`",
    "money-desc": "ORDER BY `money` DESC",
    "total": "ORDER BY `total`",
    "total-desc": "ORDER BY `total` DESC",
    "date": "ORDER BY `date`",
    "date-desc": "ORDER BY `date` DESC, `time` DESC",
    "rank": "rank",
}


class AppState:
    def __init__(self, default_user_index: int):
        self._user_index = default_user_index
        self._lock = Lock()

    @property
    def user_index(self) -> int:
        with self._lock:
            return self._user_index

    def set_user_index(self, user_index: int) -> None:
        with self._lock:
            self._user_index = user_index


def create_app(config: AppConfig | None = None) -> Flask:
    config = config or AppConfig()
    app = Flask(__name__)
    CORS(app, origins=config.cors_origins)

    database = Database(config.database)
    serial_client = SerialClient(config.serial)
    app_state = AppState(config.default_user_index)

    def on_user_identified(user_index: int) -> None:
        serial_client.write_user_id(user_index)
        app_state.set_user_index(user_index)

    face_service = FaceRecognitionService(config.face, on_user_identified)
    face_service.start()

    @app.get("/health")
    def health():
        return jsonify({"status": "ok"})

    @app.get("/get_money_data")
    def get_money_data():
        change = request.args.get("change", "")
        state = request.args.get("state", "date-desc")
        value = request.args.get("value", "")
        sort_clause = SORT_OPTIONS.get(state, SORT_OPTIONS["date-desc"])

        try:
            with database.connect() as connection:
                cursor = connection.cursor()
                original_name, user_name = _get_current_user(cursor, app_state.user_index)
                table_name = quote_identifier(original_name)
                table_state = "success"

                saved_amount = serial_client.read_saved_amount()
                if saved_amount is not None:
                    table_state = _insert_money_change(cursor, table_name, saved_amount, value)

                if change == "cost":
                    table_state = _handle_cost(cursor, table_name, value)
                elif change == "name":
                    user_name = _update_user_name(cursor, original_name, value)

                if sort_clause == "rank":
                    data = _get_rankings(cursor)
                else:
                    cursor.execute(f"SELECT * FROM {table_name} {sort_clause};")
                    data = rows_to_money_records(cursor.fetchall())

            return jsonify(
                {
                    "data": data,
                    "rate": _get_exchange_rate(config.exchange_rate_base_url, "USD", "TWD"),
                    "name": user_name,
                    "table_state": table_state,
                }
            )
        except (MySQLError, ValueError) as err:
            return jsonify({"error": str(err)}), 500

    return app


def _get_current_user(cursor, user_index: int) -> tuple[str, str]:
    cursor.execute("SELECT `original_name`, `new_name` FROM `user_name`;")
    users = cursor.fetchall()
    if user_index < 1 or user_index > len(users):
        raise ValueError(f"Invalid user index: {user_index}")
    return users[user_index - 1]


def _get_total(cursor, table_name: str) -> int:
    cursor.execute(f"SELECT `total` FROM {table_name} ORDER BY `date` DESC, `time` DESC LIMIT 1;")
    row = cursor.fetchone()
    return int(row[0]) if row and row[0] is not None else 0


def _insert_money_change(cursor, table_name: str, amount: int, limit_value: str = "") -> str:
    total = _get_total(cursor, table_name)
    if limit_value and int(limit_value) > total:
        return "fall"

    cursor.execute(
        f"""
        INSERT INTO {table_name} (`date`, `time`, `money`, `total`)
        VALUES (NOW(), NOW(), %s, (SELECT total_amount FROM (
            SELECT IFNULL(SUM(`money`), 0) + %s AS total_amount FROM {table_name}
        ) AS totals));
        """,
        (amount, amount),
    )
    return "success"


def _handle_cost(cursor, table_name: str, value: str) -> str:
    if not value:
        return "success"

    amount = int(value)
    if amount > _get_total(cursor, table_name):
        return "fall"

    return _insert_money_change(cursor, table_name, -amount)


def _update_user_name(cursor, original_name: str, value: str) -> str:
    if not value:
        return value

    cursor.execute(
        "UPDATE `user_name` SET `new_name` = %s WHERE `original_name` = %s;",
        (value, original_name),
    )
    return value


def _get_rankings(cursor) -> list[dict]:
    cursor.execute("SHOW TABLES;")
    table_names = [row[0] for row in cursor.fetchall() if row[0] != "user_name"]
    rankings = []

    for table in table_names:
        table_name = quote_identifier(table)
        cursor.execute(f"SELECT SUM(`money`) FROM {table_name};")
        total = cursor.fetchone()[0] or 0
        cursor.execute("SELECT `new_name` FROM `user_name` WHERE `original_name` = %s;", (table,))
        name_row = cursor.fetchone()
        rankings.append({"name": name_row[0] if name_row else table, "total": total})

    rankings.sort(key=lambda item: item["total"], reverse=True)
    return rankings


def _get_exchange_rate(base_url: str, base_currency: str, target_currency: str) -> float | None:
    try:
        response = requests.get(f"{base_url}/{base_currency}", timeout=5)
        response.raise_for_status()
        data = response.json()
        return data.get("rates", {}).get(target_currency)
    except requests.RequestException as err:
        print("Error fetching exchange rates:", err)
        return None


if __name__ == "__main__":
    app_config = AppConfig()
    application = create_app(app_config)
    application.run(host=app_config.host, port=app_config.port, debug=app_config.debug)
