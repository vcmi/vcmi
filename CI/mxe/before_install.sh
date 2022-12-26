#!/bin/sh

# Install nsis for installer creation
sudo add-apt-repository 'deb http://security.ubuntu.com/ubuntu bionic-security main'
sudo apt-get install -qq nsis ninja-build libssl1.0.0

# MXE repository was too slow for Travis far too often
wget -nv https://github.com/vcmi/vcmi-deps-mxe/releases/download/2022-12-26/mxe-i686-w64-mingw32.shared-2022-12-26.tar
tar -xvf mxe-i686-w64-mingw32.shared-2022-12-26.tar
sudo dpkg -i mxe-*.deb
sudo apt-get install -f --yes

if false; then
	# Add MXE repository and key
	echo "deb http://pkg.mxe.cc/repos/apt/debian wheezy main" \
		| sudo tee /etc/apt/sources.list.d/mxeapt.list

	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys D43A795B73B16ABE9643FE1AFD8FFF16DB45C6AB

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
	mxe-$MXE_TARGET-intel-tbb \
	mxe-i686-w64-mingw32.static-luajit

fi # Disable

# alias for CMake

CMAKE_LOCATION=$(which cmake)
sudo mv $CMAKE_LOCATION $CMAKE_LOCATION.orig
sudo ln -s /usr/lib/mxe/usr/bin/$MXE_TARGET-cmake $CMAKE_LOCATION
