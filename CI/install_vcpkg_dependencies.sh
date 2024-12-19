#!/usr/bin/env bash

RELEASE_TAG="v1.8"
FILENAME="dependencies-$1"
DOWNLOAD_URL="https://github.com/vcmi/vcmi-deps-windows/releases/download/$RELEASE_TAG/$FILENAME.txz"

curl -L "$DOWNLOAD_URL" | tar -xf - --xz
