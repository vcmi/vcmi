#!/usr/bin/env bash

echo "ANDROID_NDK_ROOT=$ANDROID_HOME/ndk/25.2.9519653" >> $GITHUB_ENV

brew install ninja

mkdir ~/.conan ; cd ~/.conan
curl -L "https://github.com/vcmi/vcmi-dependencies/releases/download/1.2/$DEPS_FILENAME.tgz" \
	| tar -xzf -
