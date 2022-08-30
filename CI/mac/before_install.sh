#!/usr/bin/env bash

echo DEVELOPER_DIR=/Applications/Xcode_13.4.1.app >> $GITHUB_ENV

brew install ninja

mkdir ~/.conan ; cd ~/.conan
curl -L "https://github.com/vcmi/vcmi-deps-macos/releases/latest/download/$DEPS_FILENAME.txz" \
	| tar -xf -
