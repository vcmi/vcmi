#!/bin/sh

#new Clang
sudo add-apt-repository --yes ppa:h-rayflood/llvm

#new SDL2
sudo add-apt-repository --yes ppa:zoogie/sdl2-snapshots

#new Qt
sudo add-apt-repository --yes ppa:beineri/opt-qt571-trusty

#new CMake
sudo add-apt-repository --yes ppa:george-edison55/cmake-3.x

sudo apt-get update -qq

sudo apt-get install -qq $SUPPORT
sudo apt-get install -qq $PACKAGE
sudo apt-get install -qq cmake ninja-build libboost1.54-all-dev zlib1g-dev
sudo apt-get install -qq libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
sudo apt-get install -qq libavformat-dev libswscale-dev
sudo apt-get install -qq qt57declarative
sudo apt-get install -qq libluajit-5.1-dev

#setup compiler
source /opt/qt57/bin/qt57-env.sh
export CC=${REAL_CC} CXX=${REAL_CXX}
