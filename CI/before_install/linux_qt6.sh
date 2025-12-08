#!/usr/bin/env bash

set -euo pipefail
export DEBIAN_FRONTEND=noninteractive

APT_CACHE="${APT_CACHE:-${RUNNER_TEMP:-/tmp}/apt-cache}"
sudo mkdir -p "$APT_CACHE"

sudo apt -yq -o Acquire::Retries=3 update
sudo apt -yq install eatmydata

sudo eatmydata apt -yq --no-install-recommends \
  -o Dir::Cache::archives="$APT_CACHE" \
  -o APT::Keep-Downloaded-Packages=true \
  -o Acquire::Retries=3 -o Dpkg::Use-Pty=0 \
  install \
  libboost-dev libboost-filesystem-dev libboost-system-dev libboost-thread-dev \
  libboost-program-options-dev libboost-locale-dev libboost-iostreams-dev \
  libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev \
  qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools \
  qt6-l10n-tools qt6-svg-dev \
  ninja-build zlib1g-dev libavformat-dev libswscale-dev libtbb-dev \
  libluajit-5.1-dev libminizip-dev libfuzzylite-dev libsqlite3-dev

sudo rm -f  "$APT_CACHE/lock" || true
sudo rm -rf "$APT_CACHE/partial" || true
sudo chown -R "$USER:$USER" "$APT_CACHE"
