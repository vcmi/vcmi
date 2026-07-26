#!/bin/bash
# Online SQLite backup; safe to run while lobby is up.
set -euo pipefail

OUT=/root/lobby-backup.db.gz
TMP=$(mktemp)

sqlite3 /home/lobby/.local/share/vcmi/vcmiLobby.db ".backup '$TMP'"
gzip -c "$TMP" >"$OUT"
rm -f "$TMP"

echo "Wrote $OUT"
