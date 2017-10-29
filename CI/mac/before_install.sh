#!/bin/sh

brew update
brew unlink boost
brew install boost smpeg2 innoextract libpng freetype sdl2 sdl2_ttf sdl2_image qt5 ffmpeg
brew install sdl2_mixer --with-smpeg2

export CMAKE_PREFIX_PATH="/usr/local/opt/qt5:$CMAKE_PREFIX_PATH"
