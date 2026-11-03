"""Reading and writing of data files."""

import json
import os

from .errors import DataError


def read_text(path):
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, ValueError) as error:
        raise DataError(f"{path}: {error}") from error


def load_json(path):
    try:
        return json.loads(read_text(path))
    except ValueError as error:
        raise DataError(f"{path}: {error}") from error


def write_atomic(path, text):
    """Replaces the file in one step, so that the service never reads a partially written file."""
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(text, encoding="utf-8")
    os.chmod(temporary, 0o644)
    temporary.replace(path)
