#!/bin/bash
# Restore a backup produced by export.sh. Run setup.sh first, so the package and its user exist.
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <updates-backup.tar.gz>" >&2
    exit 1
fi

BACKUP="$(readlink -f "$1")"

systemctl stop vcmi-updates
tar -xzf "$BACKUP" -C /
systemctl start vcmi-updates

vcmi-updates-release check
