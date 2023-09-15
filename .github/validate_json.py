import jstyleson
from pathlib import Path
from pprint import pprint

errors = []
for path in sorted(Path('.').glob('**/*.json')):
    # because path is an object and not a string
    path_str = str(path)
    try:
        with open(path_str, 'r') as file:
            jstyleson.load(file)
        print(f"Validation of {path_str} succeeded")
    except Exception as exc:
        print(f"Validation of {path_str} failed")
        pprint(exc)
        # https://stackoverflow.com/a/72850269/2278742
        if hasattr(exc, 'pos'):
            position_msg = f"{path_str}:{exc.lineno}:{exc.colno}"
            print(position_msg)
            errors.append({"position": position_msg, "exception": exc})

if errors:
    print("Summary of errors:")
    pprint(errors)
    raise Exception("Not all JSON files are valid")
