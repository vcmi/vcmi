#!/bin/bash
# OS-level prep for the update service host: nginx vhost and certs. Idempotent.
set -euo pipefail

apt-get update
apt-get install -y nginx python3 curl

install -d -m 755 /etc/nginx/certs
install -m 644 /root/certs/dot.vcmi.eu.pem       /etc/nginx/certs/
install -m 600 /root/certs/dot.vcmi.eu.key       /etc/nginx/certs/
install -m 644 /root/certs/cloudflare-client.crt /etc/nginx/certs/

install -m 644 "$(dirname "$0")/updates.vcmi.eu" /etc/nginx/sites-available/
ln -sfn /etc/nginx/sites-available/updates.vcmi.eu /etc/nginx/sites-enabled/updates.vcmi.eu

systemctl reload nginx
