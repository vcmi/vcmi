#!/usr/bin/env bash

sudo apt-get update
sudo apt-get install ninja-build mingw-w64 nsis
sudo update-alternatives --set i686-w64-mingw32-g++ /usr/bin/i686-w64-mingw32-g++-posix

# Workaround for getting new MinGW headers on Ubuntu 22.04.
# Remove it once MinGW headers version in repository will be 10.0 at least
curl -O -L http://mirrors.kernel.org/ubuntu/pool/universe/m/mingw-w64/mingw-w64-common_10.0.0-2_all.deb \
  && sudo dpkg -i mingw-w64-common_10.0.0-2_all.deb;
curl -O -L http://mirrors.kernel.org/ubuntu/pool/universe/m/mingw-w64/mingw-w64-i686-dev_10.0.0-2_all.deb \
  && sudo dpkg -i mingw-w64-i686-dev_10.0.0-2_all.deb;

mkdir ~/.conan ; cd ~/.conan
curl -L "https://github.com/rilian-la-te/vcmi-deps-windows-conan/releases/download/0.0.1/vcmi-deps-windows-conan.tgz" \
	| tar -xzf -
