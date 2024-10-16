#!/bin/sh

platform=$1
ARCHIVE_URL="https://github.com/smanolloff/vcmi-libtorch-builds/releases/download/v1.0/vcmi-libtorch-$platform.txz"

curl -fL "$ARCHIVE_URL" | tar -xJ -C AI/MMAI

[ -d AI/MMAI/libtorch ] || { ls -lah AI/MMAI; echo "failed to extract libtorch"; exit 1; }
