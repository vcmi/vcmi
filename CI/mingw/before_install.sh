#!/usr/bin/env bash

sudo apt-get update
sudo apt-get install ninja-build mingw-w64 nsis
sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
