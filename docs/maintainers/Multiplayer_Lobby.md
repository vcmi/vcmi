# Multiplayer lobby

- RAM requirements: 512 MB (may increase over time if lobby gets more active players)
- CPU requirements: 1 core (preferrably dedicated to ensure low latency)
- SSD requirements: up to 4 Gb (depending on log and database size over time)

Exposed to public as:

- `lobby.vcmi.eu:3031` - user login
- `beholder.vcmi.eu:3031` - old domain name for logins (deprecated, kept for VCMI 1.7.3 and older clients)
- `api.vcmi.eu` - public REST API (behind Cloudflare; nginx proxies HTTPS to lobby's plain-HTTP listener on `127.0.0.1:3032`)

- Start: `cd /home/lobby && nohup sudo -u lobby /usr/games/vcmilobby &` (also done by `setup.sh` and `upgrade.sh`)
- Stop: `killall vcmilobby` (and only fall back to `killall -9` if the process refuses to exit — `-9` can leave an SQLite WAL behind)
- Examine database (can be done live): `sqlite3 /home/lobby/.local/share/vcmi/vcmiLobby.db`
- Examine log file: `tail -n 100 /home/lobby/cache/vcmi/VCMI_Lobby_log.txt`

## Setup

All operational scripts live in [`scripts/lobby/`](scripts/lobby/). Copy the directory to `/root/lobby/` on the target server.

1. Place certificates in `/root/certs/` on the server (`dot.vcmi.eu.pem`, `dot.vcmi.eu.key`, `cloudflare-client.crt`).
2. Place the lobby `.deb` package (produced by the "Build VCMI Lobby" CI job) at any reachable path, e.g. `/root/vcmi-lobby.deb`.
3. `scp -r scripts/lobby root@new-server:/root/`
4. On the server:
   ```sh
   cd /root/lobby
   ./prepare.sh
   ./setup.sh /root/vcmi-lobby.deb
   ```

`setup.sh` creates a fresh SQLite database from the schema bundled in the .deb if one is not already present at `/home/lobby/.local/share/vcmi/vcmiLobby.db`. To restore from a previous backup instead, copy the backup into place first (or use `./import.sh` after the database exists).

## Upgrade

The upgrade is a **two-stage** process because `sqldiff` will occasionally "patch" a table by dropping and recreating it, silently losing every row. Always review the generated SQL before applying.

```sh
# Stage 1 — generate diff. Lobby keeps running; .deb is not installed yet.
./upgrade.sh /root/vcmi-lobby-new.deb
# Inspect /root/lobby-upgrade.sql. Edit out any unwanted DROP / CREATE
# TABLE rewrites — replace them with hand-written ALTER TABLE statements.

# Stage 2 — apply reviewed diff.
./upgrade.sh /root/vcmi-lobby-new.deb /root/lobby-upgrade.sql
```

Stage 2 takes a fresh backup at `/root/lobby-backup-pre-upgrade.db`, stops the running lobby (graceful first, then SIGKILL if needed), installs the .deb, applies the SQL exactly as given, and restarts.

A non-empty residual `sqldiff` after apply is **expected** — differences in column order, index naming, etc. persist without affecting correctness. The script prints them for information only and does not abort.

## Migration

1. On the old server: `./export.sh` — produces `/root/lobby-backup.db.gz`.
2. Provision the new server and run the Setup steps above using the same `.deb` package.
3. Transfer the backup directly between servers using your workstation's SSH agent for auth:
   ```sh
   ssh -A root@old-server "scp /root/lobby-backup.db.gz root@new-server:/root/"
   ```
   Or, if agent forwarding is not available, route through the workstation: `scp -3 root@old:/root/lobby-backup.db.gz root@new:/root/`.
4. On the new server:
   ```sh
   killall vcmilobby
   ./import.sh /root/lobby-backup.db.gz
   cd /home/lobby && nohup sudo -u lobby /usr/games/vcmilobby &
   ```

## Troubleshooting

- Lobby crashes on start due to `boost::filesystem::status: Permission denied [system:13]: "config"`. Solution: `cd /home/lobby` (or any other directory writable by `lobby` user)
- Lobby shut downs after logout from server: ensure that `vcmilobby` was started via `nohup`
