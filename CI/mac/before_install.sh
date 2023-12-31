#!/usr/bin/env bash

echo DEVELOPER_DIR=/Applications/Xcode_14.2.app >> $GITHUB_ENV

brew install ninja

mkdir ~/.conan ; cd ~/.conan
curl -L "https://github.com/vcmi/vcmi-deps-macos/releases/download/1.2/$DEPS_FILENAME.txz" \
	| tar -xf -
