#!/bin/sh

sudo apt-get update

# Boost
wget -nv https://boostorg.jfrog.io/artifactory/main/release/1.66.0/source/boost_1_66_0.tar.gz
tar xfz boost_1_66_0.tar.gz
cd boost_1_66_0
./bootstrap.sh --with-libraries=program_options,filesystem,system,thread,locale,date_time
./b2
sudo ./b2 install

# Dependencies
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
sudo apt-get install qtbase5-dev
sudo apt-get install ninja-build zlib1g-dev libavformat-dev libswscale-dev libtbb-dev