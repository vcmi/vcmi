#!/bin/bash
# Trigger a Discourse backup via its built-in CLI and copy the latest tarball
# to /root.
set -euo pipefail

cd /var/discourse
# 'exit' returns from the container shell that 'enter' drops into, otherwise
# the script blocks waiting on an interactive session.
./launcher enter app -c 'discourse backup; exit'

# Keep the original filename: Discourse matches backups by the name it created
# them with, so a renamed tarball cannot be restored.
BACKUP=$(ls -t shared/standalone/backups/default/*.tar.gz | head -1)
cp "$BACKUP" /root/
echo "Wrote /root/$(basename "$BACKUP")"
