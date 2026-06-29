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
  install gettext

sudo rm -f  "$APT_CACHE/lock" || true
sudo rm -rf "$APT_CACHE/partial" || true
sudo chown -R "$USER:$USER" "$APT_CACHE"
