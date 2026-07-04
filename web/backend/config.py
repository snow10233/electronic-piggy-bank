from dataclasses import dataclass, field
from pathlib import Path
import os

from dotenv import load_dotenv

BASE_DIR = Path(__file__).resolve().parents[1]
PROJECT_ROOT = BASE_DIR.parent
load_dotenv(PROJECT_ROOT / ".env")


def _path_from_env(name: str, default: Path) -> Path:
    value = Path(os.getenv(name, default))
    return PROJECT_ROOT / value if not value.is_absolute() else value


def _bool_from_env(name: str, default: bool = False) -> bool:
    value = os.getenv(name)
    if value is None:
        return default
    return value.strip().lower() in {"1", "true", "yes", "on"}


@dataclass(frozen=True)
class DatabaseConfig:
    host: str = os.getenv("DB_HOST", "localhost")
    port: int = int(os.getenv("DB_PORT", "3306"))
    user: str = os.getenv("DB_USER", "root")
    password: str = os.getenv("DB_PASSWORD", "")
    database: str = os.getenv("DB_NAME", "data")


@dataclass(frozen=True)
class SerialConfig:
    port: str = os.getenv("SERIAL_PORT", "/dev/cu.wchusbserial54E20196661")
    baudrate: int = int(os.getenv("SERIAL_BAUDRATE", "115200"))
    timeout: int = int(os.getenv("SERIAL_TIMEOUT", "2"))
    enabled: bool = _bool_from_env("SERIAL_ENABLED", True)


@dataclass(frozen=True)
class FaceConfig:
    camera_source: str = os.getenv("CAMERA_SOURCE", "0")
    cascade_path: Path = _path_from_env("FACE_CASCADE_PATH", BASE_DIR / "assets" / "face_detect.xml")
    model_path: Path = _path_from_env("FACE_MODEL_PATH", BASE_DIR / "backend" / "models" / "my.yml")
    enabled: bool = _bool_from_env("FACE_RECOGNITION_ENABLED", True)


@dataclass(frozen=True)
class AppConfig:
    host: str = os.getenv("APP_HOST", "0.0.0.0")
    port: int = int(os.getenv("APP_PORT", "5001"))
    debug: bool = _bool_from_env("FLASK_DEBUG", False)
    cors_origins: str = os.getenv("CORS_ORIGINS", "*")
    default_user_index: int = int(os.getenv("DEFAULT_USER_INDEX", "1"))
    exchange_rate_base_url: str = os.getenv("EXCHANGE_RATE_BASE_URL", "https://open.er-api.com/v6/latest")
    database: DatabaseConfig = field(default_factory=DatabaseConfig)
    serial: SerialConfig = field(default_factory=SerialConfig)
    face: FaceConfig = field(default_factory=FaceConfig)
