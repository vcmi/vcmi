import re
from pathlib import Path
from pprint import pprint

import json5
import jstyleson
import yaml

# 'json', 'json5' or 'yaml'
# json:  strict, but doesn't preserve line numbers necessarily, since it strips comments before parsing
# json5: strict and preserves line numbers even for files with line comments
# yaml:  less strict, allows e.g. leading zeros
VALIDATION_TYPE = 'json5'

errors = []
for path in sorted(Path('.').glob('**/*.json')):
    # because path is an object and not a string
    path_str = str(path)
    try:
        with open(path_str, 'r') as file:
            if VALIDATION_TYPE == 'json':
                jstyleson.load(file)
            if VALIDATION_TYPE == 'json5':
                json5.load(file)
            elif VALIDATION_TYPE == 'yaml':
                file = file.read().replace("\t", "    ")
                file = file.replace("//", "#")
                yaml.safe_load(file)
        print(f"Validation of {path_str} succeeded")
    except Exception as exc:
        print(f"Validation of {path_str} failed")
        pprint(exc)

        error_pos = path_str

        if hasattr(exc, 'pos'):
            # https://stackoverflow.com/a/72850269/2278742
            error_pos = f"{path_str}:{exc.lineno}:{exc.colno}"
            print(error_pos)
        if hasattr(exc, 'problem_mark'):
            mark = exc.problem_mark
            error_pos = f"{path_str}:{mark.line+1}:{mark.column+1}"
            print(error_pos)
        if VALIDATION_TYPE == 'json5':
            pos = re.findall(r'\d+', str(exc))
            error_pos = f"{path_str}:{pos[0]}:{pos[-1]}"

        errors.append({"error_pos": error_pos, "error_msg": exc})

if errors:
    print("Summary of errors:")
    pprint(errors)
    raise Exception("Not all JSON files are valid")
