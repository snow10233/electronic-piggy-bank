from __future__ import annotations

import serial

from .config import SerialConfig


class SerialClient:
    def __init__(self, config: SerialConfig):
        self._config = config
        self._connection: serial.Serial | None = None

    def open(self) -> None:
        if not self._config.enabled or self._connection is not None:
            return

        self._connection = serial.Serial(
            self._config.port,
            self._config.baudrate,
            timeout=self._config.timeout,
            bytesize=serial.EIGHTBITS,
            stopbits=serial.STOPBITS_ONE,
            parity=serial.PARITY_NONE,
        )

    def read_saved_amount(self) -> int | None:
        self.open()
        if self._connection is None or self._connection.in_waiting <= 0:
            return None

        raw_value = self._connection.readline().decode("utf-8").strip()
        return int(raw_value)

    def write_user_id(self, user_id: int) -> None:
        self.open()
        if self._connection is None:
            return

        self._connection.write(str(user_id).encode("utf-8"))
