#!/usr/bin/env bash

RELEASE_TAG="1.3"
FILENAME="$1"
DOWNLOAD_URL="https://github.com/vcmi/vcmi-dependencies/releases/download/$RELEASE_TAG/$FILENAME.txz"

mkdir ~/.conan
cd ~/.conan
curl -L "$DOWNLOAD_URL" | tar -xf - --xz
