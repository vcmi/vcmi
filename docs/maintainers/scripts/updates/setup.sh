#!/bin/bash
# First-time install. Provide path to the .deb produced by the "Build VCMI Update Service" CI job.
# Existing release records, channel configurations and AltStore template are preserved.
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <vcmi-updates.deb>" >&2
    exit 1
fi

DEB="$(readlink -f "$1")"
DATA=/var/lib/vcmi-updates

# Installs the vcmi-updates user and starts the service, which creates $DATA
dpkg -i "$DEB"

install -d -m 755 "$DATA/releases" "$DATA/channels"

write_channel() {
    [[ -f "$DATA/channels/$1.json" ]] || echo "$2" >"$DATA/channels/$1.json"
}
write_channel stable  '{ "source": "releases" }'
write_channel beta    '{ "source": "branch", "branch": "beta" }'
write_channel develop '{ "source": "branch", "branch": "develop" }'

[[ -f "$DATA/altstore.json" ]] || install -m 644 "$(dirname "$0")/altstore.json" "$DATA/"

vcmi-updates-release check
curl -fsS 'http://127.0.0.1:3040/autoupdate.json?version=1.8.0&os=windows&arch=x64&dist=installer'
echo
curl -fsS -o /dev/null 'http://127.0.0.1:3040/altstore.json'
