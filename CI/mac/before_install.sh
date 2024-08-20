#!/usr/bin/env bash

echo DEVELOPER_DIR=/Applications/Xcode_14.2.app >> $GITHUB_ENV

brew install ninja

. CI/install_conan_dependencies.sh "$DEPS_FILENAME"
