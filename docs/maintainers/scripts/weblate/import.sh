#!/bin/bash
# Restore volumes from a tarball produced by export.sh, then start the stack.
# docker-compose.override.yml must already be in place (setup.sh writes it).
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <weblate-backup.tgz>" >&2
    exit 1
fi

BACKUP="$1"

cd /var/weblate-docker
docker compose down 2>/dev/null || true

install -d -m 711 /var/lib/docker/volumes
tar -C /var/lib/docker/volumes -xzf "$BACKUP"

docker compose pull
docker compose up -d
