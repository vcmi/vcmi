#!/usr/bin/env bash

set -euo pipefail
export DEBIAN_FRONTEND=noninteractive

ARCH="${1:-x86_64}"
case "$ARCH" in
  x86)     triplet=i686-w64-mingw32 ;;
  x86_64)  triplet=x86_64-w64-mingw32 ;;
  *) echo "Unsupported ARCH '$ARCH' (use: x86 | x86_64)"; exit 2 ;;
esac

APT_CACHE="${APT_CACHE:-${RUNNER_TEMP:-/tmp}/apt-cache}"
sudo mkdir -p "$APT_CACHE"

sudo apt -yq -o Acquire::Retries=3 update
sudo apt -yq install eatmydata

sudo eatmydata apt -yq --no-install-recommends \
  -o Dir::Cache::archives="$APT_CACHE" \
  -o APT::Keep-Downloaded-Packages=true \
  -o Acquire::Retries=3 -o Dpkg::Use-Pty=0 \
  install \
  ninja-build nsis mingw-w64 g++-mingw-w64

if [[ -x "/usr/bin/${triplet}-g++-posix" ]]; then
  sudo update-alternatives --set "${triplet}-g++" "/usr/bin/${triplet}-g++-posix"
fi
if [[ -x "/usr/bin/${triplet}-gcc-posix" ]]; then
  sudo update-alternatives --set "${triplet}-gcc" "/usr/bin/${triplet}-gcc-posix"
fi

sudo rm -f  "$APT_CACHE/lock" || true
sudo rm -rf "$APT_CACHE/partial" || true
sudo chown -R "$USER:$USER" "$APT_CACHE"