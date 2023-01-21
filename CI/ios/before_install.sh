#!/usr/bin/env bash

echo DEVELOPER_DIR=/Applications/Xcode_14.2.app >> $GITHUB_ENV

mkdir ~/.conan ; cd ~/.conan
curl -L 'https://github.com/vcmi/vcmi-ios-deps/releases/download/1.2/ios-arm64.txz' \
	| tar -xf -
