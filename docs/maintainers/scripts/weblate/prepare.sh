#!/bin/bash
# Host prep: docker, nginx, swap, certs, weblate-docker checkout. Idempotent.
set -euo pipefail

apt-get update
apt-get install -y docker.io git docker-compose-v2 nginx

install -d -m 755 /etc/nginx/certs
install -m 644 /root/certs/dot.vcmi.eu.pem      /etc/nginx/certs/
install -m 600 /root/certs/dot.vcmi.eu.key      /etc/nginx/certs/
install -m 644 /root/certs/cloudflare-client.crt /etc/nginx/certs/

install -m 644 "$(dirname "$0")/weblate.vcmi.eu" /etc/nginx/sites-available/
ln -sfn /etc/nginx/sites-available/weblate.vcmi.eu /etc/nginx/sites-enabled/weblate.vcmi.eu
systemctl reload nginx

if [[ ! -d /var/weblate-docker ]]; then
    git clone https://github.com/WeblateOrg/docker-compose.git /var/weblate-docker
fi
