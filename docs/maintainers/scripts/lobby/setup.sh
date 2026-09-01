#!/bin/bash
# First-time lobby install. Provide path to the .deb produced by CI.
# If /home/lobby/.local/share/vcmi/vcmiLobby.db already exists, it is preserved;
# otherwise a fresh schema is created from the package's lobby.sql.
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <vcmi-lobby.deb>" >&2
    exit 1
fi

DEB="$(readlink -f "$1")"
DB=/home/lobby/.local/share/vcmi/vcmiLobby.db

apt-get install -y "$DEB"

if [[ ! -f "$DB" ]]; then
    sudo -u lobby sqlite3 "$DB" </usr/share/vcmi/config/lobby.sql
fi

cd /home/lobby
nohup sudo -u lobby /usr/games/vcmilobby >/var/log/vcmilobby.log 2>&1 &
disown
