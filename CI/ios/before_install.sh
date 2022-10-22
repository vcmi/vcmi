#!/usr/bin/env bash

echo DEVELOPER_DIR=/Applications/Xcode_13.4.1.app >> $GITHUB_ENV

mkdir ~/.conan ; cd ~/.conan
curl -L 'https://github.com/vcmi/vcmi-ios-deps/releases/download/1.1/ios-arm64.xz' \
	| tar -xf -
