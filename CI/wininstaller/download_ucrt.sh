#!/usr/bin/env bash

RELEASE_TAG="v1.9"
UCRT_FILENAME="ucrtRedist-$1"
UCRT_URL="https://github.com/vcmi/vcmi-deps-windows/releases/download/$RELEASE_TAG/$UCRT_FILENAME.txz"

UCRT_DIR="ucrt"
mkdir -p "$UCRT_DIR"
curl -L "$UCRT_URL" | tar -xf - --xz -C "$UCRT_DIR"
