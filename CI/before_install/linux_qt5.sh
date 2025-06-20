#!/bin/sh

sudo apt-get update

# Dependencies
# In case of change in dependencies list please also update:
# - developer docs at docs/developer/Building_Linux.md
# - debian build settings at debian/control
sudo apt-get install libboost-dev libboost-filesystem-dev libboost-system-dev libboost-thread-dev libboost-program-options-dev libboost-locale-dev libboost-iostreams-dev \
libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev \
qtbase5-dev qttools5-dev libqt5svg5-dev \
ninja-build zlib1g-dev libavformat-dev libswscale-dev libtbb-dev libluajit-5.1-dev \
libminizip-dev libfuzzylite-dev libsqlite3-dev # Optional dependencies
