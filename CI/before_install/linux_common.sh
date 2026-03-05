#!/usr/bin/env bash

# https://github.com/microsoft/onnxruntime/discussions/6489

ARCH=$(uname -m)
if [ "$ARCH" = "x86_64" ]; then
    ONNXRUNTIME_URL="https://github.com/microsoft/onnxruntime/releases/download/v1.18.1/onnxruntime-linux-x64-1.18.1.tgz"
elif [ "$ARCH" = "aarch64" ]; then
    ONNXRUNTIME_URL="https://github.com/microsoft/onnxruntime/releases/download/v1.18.1/onnxruntime-linux-aarch64-1.18.1.tgz"
else
    echo "Unsupported architecture: $ARCH"
    exit 1
fi

ONNXRUNTIME_ROOT=/opt/onnxruntime
sudo mkdir -p "$ONNXRUNTIME_ROOT"
curl -fsSL "$ONNXRUNTIME_URL" | sudo tar -xzv --strip-components=1 -C "$ONNXRUNTIME_ROOT"
