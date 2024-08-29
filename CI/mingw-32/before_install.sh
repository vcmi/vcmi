#!/usr/bin/env bash

sudo apt-get update
sudo apt-get install ninja-build mingw-w64 nsis
sudo update-alternatives --set i686-w64-mingw32-g++ /usr/bin/i686-w64-mingw32-g++-posix
