#!/bin/bash
# Write docker-compose.override.yml from env vars and start the stack.
# Requires (all read from env, never from argv to avoid leaking via ps):
#   WEBLATE_EMAIL_HOST           — SMTP server hostname
#   WEBLATE_EMAIL_HOST_USER      — SMTP username
#   WEBLATE_EMAIL_HOST_PASSWORD  — SMTP password
#   WEBLATE_ADMIN_EMAIL          — email for the bootstrap admin account
set -euo pipefail

: "${WEBLATE_EMAIL_HOST:?required}"
: "${WEBLATE_EMAIL_HOST_USER:?required}"
: "${WEBLATE_EMAIL_HOST_PASSWORD:?required}"
: "${WEBLATE_ADMIN_EMAIL:?required}"

cat >/var/weblate-docker/docker-compose.override.yml <<EOF
services:
  weblate:
    image: weblate/weblate:latest
    environment:
      WEBLATE_EMAIL_HOST: $WEBLATE_EMAIL_HOST
      WEBLATE_EMAIL_HOST_USER: $WEBLATE_EMAIL_HOST_USER
      WEBLATE_EMAIL_HOST_PASSWORD: $WEBLATE_EMAIL_HOST_PASSWORD
      WEBLATE_SERVER_EMAIL: weblate@vcmi.eu
      WEBLATE_DEFAULT_FROM_EMAIL: weblate@vcmi.eu
      WEBLATE_SITE_DOMAIN: weblate.vcmi.eu
      WEBLATE_ADMIN_EMAIL: $WEBLATE_ADMIN_EMAIL
      WEBLATE_ENABLE_HTTPS: 1
      WEBLATE_IP_PROXY_HEADER: HTTP_X_FORWARDED_FOR
      WEBLATE_SECURE_PROXY_SSL_HEADER: HTTP_X_FORWARDED_PROTO,https
    ports:
    - "127.0.0.1:8080:8080"
EOF

cd /var/weblate-docker
docker compose pull
docker compose up -d
