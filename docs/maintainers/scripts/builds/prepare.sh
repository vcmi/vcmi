#!/bin/bash
# Host prep: nginx + fancyindex, sshd sftp-only config for the uploader user,
# certs, and the helper scripts under /opt/vcmiscripts. Idempotent.
set -euo pipefail

apt-get update
apt-get install -y nginx libnginx-mod-http-fancyindex

install -d -m 755 /etc/nginx/certs
install -m 644 /root/certs/dot.vcmi.eu.pem            /etc/nginx/certs/
install -m 600 /root/certs/dot.vcmi.eu.key            /etc/nginx/certs/
install -m 644 /root/certs/builds.vcmi.download.pem   /etc/nginx/certs/
install -m 600 /root/certs/builds.vcmi.download.key   /etc/nginx/certs/
install -m 644 /root/certs/cloudflare-client.crt      /etc/nginx/certs/

HERE="$(dirname "$0")"

install -m 644 "$HERE/download.vcmi.eu"       /etc/nginx/sites-available/
install -m 644 "$HERE/builds.vcmi.download"   /etc/nginx/sites-available/
ln -sfn /etc/nginx/sites-available/download.vcmi.eu     /etc/nginx/sites-enabled/download.vcmi.eu
ln -sfn /etc/nginx/sites-available/builds.vcmi.download /etc/nginx/sites-enabled/builds.vcmi.download

install -m 644 "$HERE/sshd_uploader.conf" /etc/ssh/sshd_config.d/uploader-uses-sftp.conf
systemctl reload ssh

install -d -m 755 /opt/vcmiscripts
install -m 755 "$HERE/on_cron_update.sh"   /opt/vcmiscripts/
install -m 755 "$HERE/ensure_free_space.sh" /opt/vcmiscripts/
install -m 755 "$HERE/sort_builds.sh"      /opt/vcmiscripts/
