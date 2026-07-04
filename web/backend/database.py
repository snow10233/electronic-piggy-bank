import re
from contextlib import contextmanager
from typing import Iterable

import mysql.connector

from .config import DatabaseConfig


IDENTIFIER_PATTERN = re.compile(r"^[A-Za-z0-9_]+$")


class Database:
    def __init__(self, config: DatabaseConfig):
        self._config = config

    @contextmanager
    def connect(self):
        connection = mysql.connector.connect(
            host=self._config.host,
            port=self._config.port,
            user=self._config.user,
            password=self._config.password,
            database=self._config.database,
        )
        try:
            yield connection
            connection.commit()
        except Exception:
            connection.rollback()
            raise
        finally:
            connection.close()


def quote_identifier(identifier: str) -> str:
    if not IDENTIFIER_PATTERN.fullmatch(identifier):
        raise ValueError(f"Invalid database identifier: {identifier}")
    return f"`{identifier}`"


def rows_to_money_records(rows: Iterable[tuple]) -> list[dict]:
    records = []
    for row in rows:
        records.append(
            {
                "date": str(row[1]),
                "time": str(row[2]),
                "money": row[3],
                "total": row[4],
            }
        )
    return records
