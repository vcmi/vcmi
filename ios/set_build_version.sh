#!/usr/bin/env bash

today=$(date '+%Y-%m-%d')
commitShort=$(git rev-parse --short HEAD)
bundleVersion="$today-$commitShort"

/usr/libexec/PlistBuddy "$1/Info.plist" -c "Set :CFBundleVersion $bundleVersion"
