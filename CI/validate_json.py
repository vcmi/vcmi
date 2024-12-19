#!/usr/bin/env python3

import re
import sys
from pathlib import Path
from pprint import pprint

# VCMI supports JSON with comments, but not JSON5
import jstyleson 

validation_failed = False

for path in sorted(Path(".").glob("**/*.json"), key=lambda path: str(path).lower()):
    # because path is an object and not a string
    path_str = str(path)
    if path_str.startswith("."):
        continue

    try:
        with open(path_str, "r") as file:
            jstyleson.load(file)
        print(f"✅ {path_str}")
    except Exception as exc:
        print(f"❌ {path_str}: {exc}")
        validation_failed = True

if validation_failed:
    sys.exit(1)
