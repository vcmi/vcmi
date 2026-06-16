#!/bin/bash
# Restore a backup produced by export.sh. Lobby must be stopped first
# (killall vcmilobby) or the file will be overwritten while open.
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <backup.db[.gz]>" >&2
    exit 1
fi

BACKUP="$1"
DB=/home/lobby/.local/share/vcmi/vcmiLobby.db

if pgrep -x vcmilobby >/dev/null; then
    echo "vcmilobby is running; stop it first (killall vcmilobby)" >&2
    exit 1
fi

if [[ "$BACKUP" == *.gz ]]; then
    gunzip -c "$BACKUP" >"$DB"
else
    cp "$BACKUP" "$DB"
fi
chown lobby:lobby "$DB"
