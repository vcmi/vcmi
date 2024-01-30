#!/usr/bin/env bash

LATEST_XCODE=$(ls /Applications | grep Xcode | tail -n 1)
echo DEVELOPER_DIR=/Applications/$LATEST_XCODE >> $GITHUB_ENV

brew install ninja

mkdir ~/.conan ; cd ~/.conan
curl -L "https://github.com/vcmi/vcmi-deps-macos/releases/download/1.2/$DEPS_FILENAME.txz" \
	| tar -xf -
