#!/usr/bin/env bash

RELEASE_TAG="2025-12-10"
FILENAME="$1.tgz"
DOWNLOAD_URL="https://github.com/vcmi/vcmi-dependencies/releases/download/$RELEASE_TAG/$FILENAME"

downloadedFile="$RUNNER_TEMP/$FILENAME"
curl -Lo "$downloadedFile" "$DOWNLOAD_URL"
conan cache restore "$downloadedFile"
