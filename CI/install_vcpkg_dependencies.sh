#!/usr/bin/env bash

ACCOUNT="vcmi"

# Fetch latest release tag from GitHub API
# RELEASE_TAG=$(curl -s "https://api.github.com/repos/$ACCOUNT/vcmi-deps-windows/releases/latest" | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/')

RELEASE_TAG="v1.9"

DEP_FILENAME="dependencies-$1"
DEP_URL="https://github.com/$ACCOUNT/vcmi-deps-windows/releases/download/$RELEASE_TAG/$DEP_FILENAME.txz"

curl -L "$DEP_URL" | tar -xf - --xz

UCRT_FILENAME="ucrtRedist-$1"
UCRT_URL="https://github.com/$ACCOUNT/vcmi-deps-windows/releases/download/$RELEASE_TAG/$UCRT_FILENAME.txz"

mkdir -p ucrt
curl -L "$UCRT_URL" | tar -xf - --xz -C ucrt
