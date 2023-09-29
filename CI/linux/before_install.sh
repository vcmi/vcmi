#!/bin/sh

sudo apt-get update

# Dependencies
sudo apt-get install libboost-all-dev \
libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev \
qtbase5-dev \
ninja-build zlib1g-dev libavformat-dev libswscale-dev libtbb-dev libluajit-5.1-dev \
libminizip-dev libfuzzylite-dev qttools5-dev # Optional dependencies
