#!/bin/bash
# OS-level prep for the lobby host: nginx, sqlite tooling, certs, user.
set -euo pipefail

apt-get update
apt-get install -y nginx sqlite3 sqlite3-tools

# Cert install (operator placed them in /root/certs/ before running this).
install -d -m 755 /etc/nginx/certs
install -m 644 /root/certs/dot.vcmi.eu.pem      /etc/nginx/certs/
install -m 600 /root/certs/dot.vcmi.eu.key      /etc/nginx/certs/
install -m 644 /root/certs/cloudflare-client.crt /etc/nginx/certs/

install -m 644 "$(dirname "$0")/api.vcmi.eu" /etc/nginx/sites-available/
ln -sfn /etc/nginx/sites-available/api.vcmi.eu /etc/nginx/sites-enabled/api.vcmi.eu

# Idempotent user creation.
id -u lobby >/dev/null 2>&1 || useradd -m lobby
sudo -u lobby mkdir -p /home/lobby/.local/share/vcmi/

systemctl reload nginx
