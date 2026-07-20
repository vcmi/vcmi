#!/bin/bash
# Restore a backup produced by export.sh. Discourse must already be running
# (i.e. setup.sh completed) so that the restore command exists in the container.
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <discourse-backup.tar.gz>" >&2
    exit 1
fi

BACKUP="$1"
BACKUP_NAME=$(basename "$BACKUP")
DEST=/var/discourse/shared/standalone/backups/default

mkdir -p "$DEST"
cp "$BACKUP" "$DEST/$BACKUP_NAME"

cd /var/discourse
# Trailing 'exit' returns from the container shell that 'enter' drops into.
./launcher enter app -c "discourse enable_restore && discourse restore $BACKUP_NAME && discourse disable_restore; exit"
