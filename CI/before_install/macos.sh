#!/usr/bin/env bash

echo DEVELOPER_DIR=/Applications/Xcode_26.2.app >> $GITHUB_ENV

brew untap aws/tap || true
brew trust --formula azure/bicep/bicep || true
brew update
