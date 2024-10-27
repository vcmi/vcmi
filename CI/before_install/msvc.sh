#!/usr/bin/env bash

DUMPBIN_DIR=$(vswhere -latest -find **/dumpbin.exe | head -n 1)
dirname "$DUMPBIN_DIR" > $GITHUB_PATH
