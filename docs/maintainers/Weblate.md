# Weblate

- RAM requirements: 4 GB, may run at 2 Gb, but performance is poor
- CPU requirements: 2 core, may run at 1 core but performance is poor
- SSD requirements: over 20 GB. Will grow over time with more translations

Accessible as [weblate.vcmi.eu](https://weblate.vcmi.eu/)

Admin access:

- Ivan Savenko

Additional accounts can be given admin rights via admin panel.

## Configuration

- Located at `/var/weblate-docker`.
- Configuration file is at `/var/weblate-docker/docker-compose.override.yml`
- Administration primarily via web interface

All operational scripts live in [`scripts/weblate/`](scripts/weblate/). Copy
that directory to `/root/weblate/` on the target server before running anything.

## Setup

Reference: [official Weblate docker docs](https://docs.weblate.org/en/latest/admin/install/docker.html).

1. On your workstation, place the Cloudflare Origin certificate + key and the
   Cloudflare authenticated‑pull root cert in a local `certs/` directory.
2. `scp -r certs/ root@new-server:/root/`
3. `scp -r scripts/weblate root@new-server:/root/`
4. On the server:
   ```sh
   cd /root/weblate
   ./prepare.sh
   WEBLATE_EMAIL_HOST=...           \
   WEBLATE_EMAIL_HOST_USER=...      \
   WEBLATE_EMAIL_HOST_PASSWORD=...  \
   WEBLATE_ADMIN_EMAIL=...          \
       ./setup.sh
   ```

`prepare.sh` installs docker / nginx / certs / swap / the upstream
`weblate-docker` git clone. `setup.sh` writes
`/var/weblate-docker/docker-compose.override.yml` from the four environment
variables and starts the stack.

## Upgrade

`./upgrade.sh` pulls latest images and restarts the stack:

```sh
cd /root/weblate
./upgrade.sh
docker compose -f /var/weblate-docker/docker-compose.yml logs -f weblate
```

If the new container crash-loops, redeploying the previous tag is enough as long as the PostgreSQL major version did not change. When upstream bumps PG (e.g. 15 → 16), the on-disk `postgres-data` volume becomes incompatible — the Weblate release notes call this out and link a dump/restore procedure; follow it before re-running `up -d`. Pinning a specific tag in `docker-compose.override.yml` instead of `:latest` makes such bumps a deliberate action rather than a surprise.

## Migration

Source and target host must run the **same Weblate image tag** — the bundled PostgreSQL major version must match the on-disk data files. Treat "move host" and "upgrade Weblate" as two separate operations.

Persistent state lives in two Docker named volumes:

- `weblate-docker_weblate-data` — uploads, VCS clones, SSH keys for git push
- `weblate-docker_postgres-data` — the database

`weblate-docker_weblate-cache` and `weblate-docker_redis-data` are intentionally skipped: the cache regenerates on first access, and Celery jobs are idempotent on restart.

1. On the old server: `./export.sh` — produces `/root/weblate-backup.tgz` and leaves the stack stopped.
2. Provision the new server and copy certs + `scripts/weblate/` to it (same as Setup steps 1–3 above).
3. Copy `docker-compose.override.yml` from the old server to the new one, **or** re-run `setup.sh` with the same env vars (it is idempotent and ends in `docker compose up -d`, which `import.sh` then overrides).
4. Transfer the backup tarball directly between servers (using the workstation's SSH agent for auth — no key trust needed between the two hosts):
   ```sh
   ssh -A root@old-server "scp /root/weblate-backup.tgz root@new-server:/root/"
   ```
   Or, if agent forwarding is not available, route through the workstation: `scp -3 root@old:/root/weblate-backup.tgz root@new:/root/`.
5. On the new server:
   ```sh
   cd /root/weblate
   ./prepare.sh
   ./import.sh /root/weblate-backup.tgz
   ```
6. Switch the DNS A record for `weblate.vcmi.eu` to the new IP.

### Troubleshooting

- **`The /app/data volume is not writable` on startup.** Volume contents must be numerically owned by UID 1000. `export.sh`/`import.sh` use `tar cz`/`tar xz`, which preserve it; `rsync` without `-a` or `cp -r` do not. Fix:
  ```sh
  chown -R 1000:1000 /var/lib/docker/volumes/weblate-docker_weblate-data/_data
  ```

- **Email send "timed out" after migration.** Almost always the hosting provider blocking outbound 25 / 465 / 587 on the new IP (most VPS providers block outbound SMTP by default on fresh accounts). Test from the new host:
  ```sh
  nc -zv <WEBLATE_EMAIL_HOST> 587
  ```
  If it hangs, open a ticket with the provider to lift the SMTP block. Second-most-common cause is an IPv6 black hole — `nc -4 -zv` works but `nc -6 -zv` hangs; either fix v6 routing or pin the SMTP host to its v4 address.
