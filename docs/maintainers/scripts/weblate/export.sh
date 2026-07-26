#!/bin/bash
# Stop the stack and tar the two persistent volumes.
# Skip the redis-data volume (Celery queue, idempotent on restart) and
# weblate-cache (regenerates on first access).
set -euo pipefail

OUT=/root/weblate-backup.tgz

cd /var/weblate-docker
docker compose down

tar -C /var/lib/docker/volumes -czf "$OUT" \
    weblate-docker_weblate-data \
    weblate-docker_postgres-data

echo "Wrote $OUT"
echo "Note: stack is now stopped. Re-run 'docker compose up -d' if you wanted a hot snapshot."
