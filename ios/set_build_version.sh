#!/usr/bin/env bash

today=$(date '+%Y-%m-%d')
commitShort=$(git rev-parse --short HEAD)
bundleVersion="$today-$commitShort"

/usr/libexec/PlistBuddy "$1/Info.plist" -c "Set :CFBundleVersion $bundleVersion"

/usr/libexec/PlistBuddy "$1/Settings.bundle/Root.plist" -c "Set :PreferenceSpecifiers:0:DefaultValue $MARKETING_VERSION"
/usr/libexec/PlistBuddy "$1/Settings.bundle/Root.plist" -c "Set :PreferenceSpecifiers:1:DefaultValue $bundleVersion"
