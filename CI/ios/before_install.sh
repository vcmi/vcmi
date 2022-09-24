#!/usr/bin/env bash

echo DEVELOPER_DIR=/Applications/Xcode_13.4.1.app >> $GITHUB_ENV

curl -L 'https://github.com/vcmi/vcmi-ios-deps/releases/latest/download/vcmi-ios-depends-xc13.2.1.txz' \
	| tar -xf -
build/fix_install_paths.command
