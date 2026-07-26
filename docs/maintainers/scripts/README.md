# Server management scripts

Each subdirectory is self-contained: copy the whole directory to `/root` on the
target server and run the scripts from there.

## Layout

| Directory | Service | Host |
| --------- | ------- | ---- |
| `lobby/` | Multiplayer lobby + `api.vcmi.eu` | `vcmi-lobby` |
| `weblate/` | Weblate translation server | `vcmi-web` |
| `discourse/` | Forum | `vcmi-forum` |
| `builds/` | Daily builds upload/download + nginx | `vcmi-web` |
| `web/` | Redirect vhosts (`wiki.vcmi.eu`, `slack.vcmi.eu`, `bugs.vcmi.eu`) | `vcmi-web` |

## Per-service scripts

Each directory contains a subset of:

- `prepare.sh` — install OS-level deps (nginx, docker, certificates, users, …).
  Idempotent; safe to re-run.
- `setup.sh` — first-time service configuration. Reads secrets from environment
  variables; prints a usage block if any required vars are missing.
- `export.sh` — produce a backup tarball / dump into `/root/<service>-backup.<ext>`.
- `import.sh <backup-file>` — restore a backup produced by `export.sh`.
- `upgrade.sh` — in-place upgrade of the running service.

Operations that don't apply to a given service simply don't have a script.

## Certificates

Certificates are copied **manually** to `/root/certs/` on each server before
running `prepare.sh`. Each `prepare.sh` only installs the certs that its
service uses:

| File | Used by |
| ---- | ------- |
| `cloudflare-client.crt` | every nginx vhost (Cloudflare authenticated origin pull) |
| `dot.vcmi.eu.pem`, `dot.vcmi.eu.key` | all `*.vcmi.eu` services |
| `forum.vcmi.eu.pem`, `forum.vcmi.eu.key` | `discourse/` |
| `builds.vcmi.download.pem`, `builds.vcmi.download.key` | `builds/` |

## Typical workflows

**Fresh install:**

```sh
# from your workstation
scp -r certs/ root@<new-server>:/root/
scp -r <service>/ root@<new-server>:/root/

ssh root@<new-server>
cd /root/<service>
./prepare.sh
EXAMPLE_REQUIRED_VAR=... ./setup.sh
```

**Migration (host A → host B):**

```sh
# on host A
cd /root/<service>
./export.sh                              # writes /root/<service>-backup.<ext>

# from workstation
scp -r certs/ root@B:/root/
scp -r <service>/ root@B:/root/

# transfer the backup directly between servers (no workstation hop).
# Agent forwarding (`ssh -A`) is what lets host A authenticate to host B
# using your local SSH agent — neither server needs the other's key.
ssh -A root@A "scp /root/<service>-backup.* root@B:/root/"

# on host B
cd /root/<service>
./prepare.sh
./import.sh /root/<service>-backup.<ext>
```

If you prefer to route through your workstation (slower but doesn't require
agent forwarding to be enabled): `scp -3 root@A:/root/<service>-backup.* root@B:/root/`.

**Upgrade:**

```sh
cd /root/<service>
./upgrade.sh
```

## Notes

- Scripts assume they are run as `root`.
- Scripts use `set -euo pipefail`; a failure aborts immediately.
- Secrets are taken from environment variables, never from command-line
  arguments, so they don't leak via `ps`, shell history, or
  `/proc/<pid>/cmdline`.
