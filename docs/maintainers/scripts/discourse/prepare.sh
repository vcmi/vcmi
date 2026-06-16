#!/bin/bash
# Install nginx, Discourse runtime, and certs. Idempotent.
set -euo pipefail

apt-get update
apt-get install -y nginx

install -d -m 755 /etc/nginx/certs
install -m 644 /root/certs/forum.vcmi.eu.pem    /etc/nginx/certs/
install -m 600 /root/certs/forum.vcmi.eu.key    /etc/nginx/certs/
install -m 644 /root/certs/cloudflare-client.crt /etc/nginx/certs/

install -m 644 "$(dirname "$0")/forum.vcmi.eu" /etc/nginx/sites-available/
ln -sfn /etc/nginx/sites-available/forum.vcmi.eu /etc/nginx/sites-enabled/forum.vcmi.eu
systemctl reload nginx

if [[ ! -d /var/discourse ]]; then
    wget -qO- https://raw.githubusercontent.com/discourse/discourse_docker/main/install-discourse | bash
fi
