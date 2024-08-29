#!/usr/bin/env bash

echo DEVELOPER_DIR=/Applications/Xcode_14.2.app >> $GITHUB_ENV

. CI/install_conan_dependencies.sh "dependencies-ios"

brew install ninja
