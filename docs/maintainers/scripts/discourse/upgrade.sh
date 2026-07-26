#!/bin/bash
# Rebuild the Discourse container — pulls latest base image and applies any
# app.yml changes. Causes ~5min of downtime.
set -euo pipefail

cd /var/discourse
./launcher rebuild app
