#!/bin/sh

sudo apt-get update

# Dependencies
sudo apt-get install libboost-all-dev
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
sudo apt-get install qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools
sudo apt-get install ninja-build zlib1g-dev libavformat-dev libswscale-dev libtbb-dev libluajit-5.1-dev
# Optional dependencies
sudo apt-get install libminizip-dev libfuzzylite-dev
# `gear-connector` dependencies
sudo apt-get install libgtk-3-dev libjavascriptcoregtk-4.0 libsoup2.4-dev libwebkit2gtk-4.0
