# Discourse

- RAM requirements: ~1.5 GB.
- CPU requirements: 1 core
- SSD requirements: ~20 GB. May be more over time

Accessible as [forum.vcmi.eu](https://forum.vcmi.eu/)

Admin access:

- Ivan Savenko
- AVS
- SXX
- Warmonger

Additional accounts can be given admin rights via admin panel. Or, if none are available - directly via server console (requires root access to server).

## Configuration

- Located at `/var/discourse`.
- Configuration file is at `/var/discourse/containers/app.yml`. The template lives in [`scripts/discourse/app.yml`](scripts/discourse/) and is rendered from environment variables by `setup.sh`.
- For administration typical approach is `cd /var/discourse; ./launcher enter app`. However most commands are also available in web UI.

References:

- [Official install docs](https://github.com/discourse/discourse/blob/main/docs/INSTALL-cloud.md)
- [nginx multi-server configuration](https://meta.discourse.org/t/run-other-websites-on-the-same-machine-as-discourse/17247)

## Setup

All operational scripts live in [`scripts/discourse/`](scripts/discourse/). Copy the directory to `/root/discourse/` on the target server.

1. Place certificates in `/root/certs/` on the server (`forum.vcmi.eu.pem`, `forum.vcmi.eu.key`, `cloudflare-client.crt`).
2. `scp -r scripts/discourse root@new-server:/root/`
3. On the server:
   ```sh
   cd /root/discourse
   ./prepare.sh
   DISCOURSE_DEVELOPER_EMAILS='saven.ivan@gmail.com'  \
   DISCOURSE_SMTP_USER_NAME='...'                     \
   DISCOURSE_SMTP_PASSWORD='...'                      \
       ./setup.sh
   ```

`prepare.sh` installs nginx + certs and runs the official Discourse install script (downloads Docker, sets up `/var/discourse`). `setup.sh` renders the templated `app.yml`, writes it to `/var/discourse/containers/app.yml`, and runs `./launcher rebuild app`.

The bundled `app.yml` runs Discourse via the `web.socketed.template.yml` Unix socket and disables Discourse's own SSL templates — HTTPS is terminated by host nginx using the Cloudflare Origin cert.

## Upgrade

`./upgrade.sh` runs `./launcher rebuild app`. Causes ~5 minutes of downtime.

Alternatively can be done in the admin UI, but untested.

## Migration

DNS cutover goes **last** to minimise visible downtime — the new instance is
fully restored and serving before users are pointed at it. (Discourse itself
knows its hostname from `DISCOURSE_HOSTNAME` in `app.yml`, not from DNS, so a
restore against the new container works before the A record moves.)

1. On the old server: `./export.sh` — produces `/root/discourse-backup.tar.gz`.
2. Set up Discourse on the new server (Setup steps above).
3. Transfer the backup directly between servers using your workstation's SSH agent for auth:
   ```sh
   ssh -A root@old-server "scp /root/discourse-backup.tar.gz root@new-server:/root/"
   ```
   Or, if agent forwarding is not available, route through the workstation: `scp -3 root@old:/root/discourse-backup.tar.gz root@new:/root/`.
4. On the new server: `./import.sh /root/discourse-backup.tar.gz`.
5. Switch the DNS A record for `forum.vcmi.eu` to the new server IP.
