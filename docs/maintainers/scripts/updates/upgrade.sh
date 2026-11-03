#!/bin/bash
# In-place upgrade. Data in /var/lib/vcmi-updates is not part of the package and is kept.
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <vcmi-updates.deb>" >&2
    exit 1
fi

# Restarts the service
dpkg -i "$(readlink -f "$1")"

vcmi-updates-release check
