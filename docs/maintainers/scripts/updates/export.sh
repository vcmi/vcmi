#!/bin/bash
# Backs up release records, channel configurations, AltStore template and configuration.
set -euo pipefail

OUT=/root/updates-backup.tar.gz

tar -czf "$OUT" -C / var/lib/vcmi-updates etc/vcmi-updates.conf

echo "Wrote $OUT"
