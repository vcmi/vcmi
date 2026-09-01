#!/bin/bash
# Render app.yml from env vars and rebuild the Discourse container.
# Requires (from env, never argv):
#   DISCOURSE_DEVELOPER_EMAILS  — comma-separated list of bootstrap admin emails
#   DISCOURSE_SMTP_USER_NAME    — Mailgun SMTP user
#   DISCOURSE_SMTP_PASSWORD     — Mailgun SMTP password
set -euo pipefail

: "${DISCOURSE_DEVELOPER_EMAILS:?required}"
: "${DISCOURSE_SMTP_USER_NAME:?required}"
: "${DISCOURSE_SMTP_PASSWORD:?required}"

# envsubst replaces the four ${...} placeholders only; everything else
# (including Discourse's own $home in the hook) is preserved.
export DISCOURSE_DEVELOPER_EMAILS DISCOURSE_SMTP_USER_NAME DISCOURSE_SMTP_PASSWORD
envsubst '$DISCOURSE_DEVELOPER_EMAILS $DISCOURSE_SMTP_USER_NAME $DISCOURSE_SMTP_PASSWORD' \
    <"$(dirname "$0")/app.yml" >/var/discourse/containers/app.yml

cd /var/discourse
./launcher rebuild app
