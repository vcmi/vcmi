class DataError(Exception):
    """Invalid release record, channel configuration, build manifest or changelog."""


def check_fields(data, source, required, optional=()):
    """Checks that data is an object with all required fields and no unknown fields."""
    if not isinstance(data, dict):
        raise DataError(f"{source}: expected an object")
    for name in required:
        if name not in data:
            raise DataError(f"{source}: missing field '{name}'")
    for name in data:
        if name not in required and name not in optional:
            raise DataError(f"{source}: unknown field '{name}'")
