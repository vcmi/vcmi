#!/bin/bash
# Two-stage in-place upgrade.
#
# Stage 1 (generate diff for review):
#     ./upgrade.sh <vcmi-lobby.deb>
#   Extracts the new package's lobby.sql without installing the .deb,
#   computes the schema diff vs the live DB, writes it to
#   /root/lobby-upgrade.sql, prints it, and exits. The lobby keeps running
#   unchanged.
#
# Stage 2 (apply reviewed diff):
#     ./upgrade.sh <vcmi-lobby.deb> /root/lobby-upgrade.sql
#   Backs up the DB, stops lobby, installs the .deb, applies the SQL
#   exactly as reviewed, restarts.
#
# Why two stages: sqldiff sometimes "patches" a table by recreating it,
# which silently drops every row. Always read the generated SQL before
# stage 2 and edit out any unwanted DROP / CREATE TABLE rewrites.
#
# A non-empty residual sqldiff after apply is expected — differences in
# column order, index naming, etc. persist without affecting correctness.
# It is printed for information only and never aborts the upgrade.
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "Usage:" >&2
    echo "  $0 <vcmi-lobby.deb>                    # stage 1: generate diff" >&2
    echo "  $0 <vcmi-lobby.deb> <reviewed.sql>     # stage 2: apply" >&2
    exit 1
fi

DEB="$(readlink -f "$1")"
DIFF_SQL="${2:-}"
DB=/home/lobby/.local/share/vcmi/vcmiLobby.db
DEFAULT_DIFF=/root/lobby-upgrade.sql
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# Pull the new schema from the .deb without installing — stage 1 must not
# leave the on-disk binary newer than the DB if the operator never runs
# stage 2 (a subsequent restart would boot new code against the old DB).
dpkg-deb -x "$DEB" "$WORK/extracted"
sqlite3 "$WORK/schema.db" <"$WORK/extracted/usr/share/vcmi/config/lobby.sql"

if [[ -z "$DIFF_SQL" ]]; then
    # --- Stage 1: produce diff, exit ---
    sqldiff --schema "$DB" "$WORK/schema.db" >"$DEFAULT_DIFF"

    if [[ ! -s "$DEFAULT_DIFF" ]]; then
        echo "Schema already matches; no upgrade SQL needed."
        rm -f "$DEFAULT_DIFF"
        exit 0
    fi

    echo "Generated $DEFAULT_DIFF:"
    echo "----"
    cat "$DEFAULT_DIFF"
    echo "----"
    echo "Review the SQL above before applying — sqldiff occasionally rewrites"
    echo "whole tables (DROP + CREATE), which silently drops every row."
    echo "When satisfied, run:"
    echo "  $0 $1 $DEFAULT_DIFF"
    exit 0
fi

# --- Stage 2: backup, stop, install, apply, restart ---
sqlite3 "$DB" ".backup '/root/lobby-backup-pre-upgrade.db'"

killall vcmilobby 2>/dev/null || true
# Wait for clean exit; SIGKILL is a last resort because it can leave SQLite WAL.
for _ in {1..10}; do
    pgrep -x vcmilobby >/dev/null || break
    sleep 1
done
pgrep -x vcmilobby >/dev/null && killall -9 vcmilobby || true

apt-get install -y "$DEB"
sqlite3 "$DB" <"$DIFF_SQL"

RESIDUAL=$(sqldiff --schema "$DB" "$WORK/schema.db" || true)
if [[ -n "$RESIDUAL" ]]; then
    echo "Residual schema diff after apply (informational — column order,"
    echo "index naming, etc. can differ without affecting correctness):"
    echo "$RESIDUAL"
fi

cd /home/lobby
nohup sudo -u lobby /usr/games/vcmilobby >/var/log/vcmilobby.log 2>&1 &
disown
