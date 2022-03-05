#!/bin/sh

brew update
brew pin python@3.9
brew install smpeg2 libpng freetype qt5 ffmpeg@4 ninja boost tbb luajit
brew install sdl2 sdl2_ttf sdl2_image sdl2_mixer

echo CMAKE_PREFIX_PATH="/usr/local/opt/ffmpeg@4:/usr/local/opt/qt5:$CMAKE_PREFIX_PATH" >> $GITHUB_ENV
