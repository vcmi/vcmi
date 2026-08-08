#!/usr/bin/env bash

RELEASE_TAG="2026-08-14"
FILENAME="$1.txz"
DOWNLOAD_URL="https://github.com/Laserlicht/vcmi-dependencies/releases/download/$RELEASE_TAG/$FILENAME"

downloadedFile="$RUNNER_TEMP/$FILENAME"
curl -Lo "$downloadedFile" "$DOWNLOAD_URL"
conan cache restore "$downloadedFile"
