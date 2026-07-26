#!/bin/bash
# Install nginx and the three redirect vhosts (wiki / slack / bugs).
# These currently sit on vcmi-web alongside builds; this directory exists
# as a separate unit so they can be migrated independently if needed.
set -euo pipefail

apt-get update
apt-get install -y nginx

install -d -m 755 /etc/nginx/certs
install -m 644 /root/certs/dot.vcmi.eu.pem      /etc/nginx/certs/
install -m 600 /root/certs/dot.vcmi.eu.key      /etc/nginx/certs/
install -m 644 /root/certs/cloudflare-client.crt /etc/nginx/certs/

HERE="$(dirname "$0")"
for site in wiki.vcmi.eu slack.vcmi.eu bugs.vcmi.eu; do
    install -m 644 "$HERE/$site" /etc/nginx/sites-available/
    ln -sfn /etc/nginx/sites-available/$site /etc/nginx/sites-enabled/$site
done

systemctl reload nginx
