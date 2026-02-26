FROM monkeyx/retro_builder:arm64
WORKDIR /usr/local/app

ENV DEBIAN_FRONTEND=noninteractive

# from VCMI build docs
RUN apt-get update && apt-get install -y cmake g++ clang libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev zlib1g-dev libavformat-dev libswscale-dev libboost-dev libboost-filesystem-dev libboost-system-dev libboost-thread-dev libboost-program-options-dev libboost-locale-dev libboost-iostreams-dev qtbase5-dev libtbb-dev libluajit-5.1-dev liblzma-dev libsqlite3-dev libminizip-dev qttools5-dev ninja-build ccache

# newer cmake version to support presets
RUN apt-get remove -y cmake
RUN apt-get install -y libssl-dev
RUN wget https://github.com/Kitware/CMake/releases/download/v3.31.5/cmake-3.31.5.tar.gz ; tar zxvf cmake-3.31.5.tar.gz ; cd cmake-3.31.5 ; ./bootstrap ; make ; make install ; cd .. ; rm -rf cmake-3.31.5

CMD ["sh", "-c", " \
    # switch to mounted dir
    cd /vcmi ; \
    # fix for wrong path of base image
    ln -s /usr/lib/libSDL2.so /usr/lib/aarch64-linux-gnu/libSDL2.so ; \
    # build
    cmake --preset portmaster-release ; \
    cmake --build --preset portmaster-release ; \
    # export missing libraries
    ldd /vcmi/out/build/portmaster-release/bin/vcmiclient | grep -e libboost -e libtbb -e libicu | awk 'NF == 4 { system(\"cp \" $3 \" /vcmi/out/build/portmaster-release/bin/\") }' \
"]

# Build on ARM64 processor or ARM64 chroot with:
#      docker build -f docker/BuildPortmaster-aarch64.dockerfile -t vcmi-portmaster-build .
#      docker run -it --rm -v $PWD/:/vcmi vcmi-portmaster-build