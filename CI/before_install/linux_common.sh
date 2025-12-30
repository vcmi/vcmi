#!/usr/bin/env bash

# https://github.com/microsoft/onnxruntime/discussions/6489
ONNXRUNTIME_URL=https://github.com/microsoft/onnxruntime/releases/download/v1.18.1/onnxruntime-linux-x64-1.18.1.tgz
ONNXRUNTIME_ROOT=/opt/onnxruntime
sudo mkdir -p "$ONNXRUNTIME_ROOT"
curl -fsSL "$ONNXRUNTIME_URL" | sudo tar -xzv --strip-components=1 -C "$ONNXRUNTIME_ROOT"
