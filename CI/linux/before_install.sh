#!/bin/sh

sudo apt-get update

# Dependencies
sudo apt-get install libboost-all-dev
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
sudo apt-get install qtbase5-dev
sudo apt-get install ninja-build zlib1g-dev libavformat-dev libswscale-dev libtbb-dev libluajit-5.1-dev
# Optional dependencies
sudo apt-get install libminizip-dev libfuzzylite-dev qttools5-dev enet
