#!/bin/bash
# Re-pull images (override file pins :latest) and restart.
# A PostgreSQL major bump in the new image will refuse to start against the
# existing postgres-data volume — Weblate release notes call this out.
set -euo pipefail

cd /var/weblate-docker
git pull
docker compose pull
docker compose down
docker compose up -d
