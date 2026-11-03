#!/usr/bin/env bash

scriptDir=$(dirname "${BASH_SOURCE[0]}")

. "$scriptDir/macos-common.sh"
echo DEVELOPER_DIR=/Applications/Xcode_27.1.app >> $GITHUB_ENV
