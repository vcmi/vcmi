#!/usr/bin/env bash

sudo apt-get update
sudo apt-get install ninja-build

mkdir ~/.conan ; cd ~/.conan
curl -L "https://github.com/vcmi/vcmi-dependencies/releases/download/android-1.0/$DEPS_FILENAME.txz" \
	| tar -xf - --xz
