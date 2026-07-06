#!/usr/bin/env bash

scriptDir=$(dirname "${BASH_SOURCE[0]}")

. "$scriptDir/macos-common.sh"
brew bundle
