#!/bin/sh

# Install nsis for installer creation
sudo apt-get install -qq nsis ninja-build

# MXE repository was too slow for Travis far too often
wget https://github.com/vcmi/vcmi-deps-mxe/releases/download/2019-06-28/mxe-i686-w64-mingw32.shared-2019-06-28.tar
tar -xvf mxe-i686-w64-mingw32.shared-2019-06-28.tar
sudo dpkg -i mxe-*.deb
sudo apt-get install -f --yes

if false; then
# Add MXE repository and key
echo "deb http://pkg.mxe.cc/repos/apt trusty main" \
    | sudo tee /etc/apt/sources.list.d/mxeapt.list

sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 86B72ED9

# Install needed packages
sudo apt-get update -qq

sudo apt-get install -q --yes \
mxe-$MXE_TARGET-gcc \
mxe-$MXE_TARGET-boost \
mxe-$MXE_TARGET-zlib \
mxe-$MXE_TARGET-sdl2 \
mxe-$MXE_TARGET-sdl2-gfx \
mxe-$MXE_TARGET-sdl2-image \
mxe-$MXE_TARGET-sdl2-mixer \
mxe-$MXE_TARGET-sdl2-ttf \
mxe-$MXE_TARGET-ffmpeg \
mxe-$MXE_TARGET-qt \
mxe-$MXE_TARGET-qtbase \
mxe-i686-w64-mingw32.static-luajit

fi # Disable

# alias for CMake
sudo mv /usr/bin/cmake /usr/bin/cmake.orig
sudo ln -s /usr/lib/mxe/usr/bin/$MXE_TARGET-cmake /usr/bin/cmake
