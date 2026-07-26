# Builds downloads server

- RAM requirements: non-existing
- CPU requirements: non-existing
- SSD requirements: 100 GB+, can be located on a separate volume

For storage we need around 50 Gb to store 1 month worth of builds. Actual amount that we can store varies with activity - each accepted PR means one more build to store. Amount that we want to store also depends - they are rarely needed, but we had several cases where we had to track faulty build, so amount nedeed is maximum time between two major releases (up to ~6 months)

## Configuration

Contains two users:

- `uploader`: can only use sftp uploads. Private key for login is stored as Github Secret in vcmi/vcmi. Also available in `/root/.ssh/uploader` on server
- `downloader`: used as owner for files available to download for public

Important file locations:

- `/opt/vcmiscripts/` - contains the cron-driven helper scripts (also mirrored in [`scripts/builds/`](scripts/builds/))
- `/home/uploader/uploads` - incoming directory for files uploaded from Github Actions
- `/home/downloader/tmp` - temporary storage for files that have finished upload but yet to be sorted. Generally this directory should be empty except for tiny moments of time while script is running OR if script fails to sort some builds such as builds for new platforms with unexpected file names
- `/home/downloader/www` - permanent storage for files available to download via nginx by players

Cron-driven helpers (run by `cron` every minute as root):

- `on_cron_update.sh` - moves builds that finished upload from `/home/uploader/uploads` to `/home/downloader/tmp`, then runs `ensure_free_space.sh` and `sort_builds.sh` as user `downloader`.
- `ensure_free_space.sh` - automatically removes oldest builds from `/home/downloader/www/`, except for releases in `/home/downloader/www/branch/master`.
- `sort_builds.sh` - renames and moves builds from `/home/downloader/tmp` to various directories in `/home/downloader/www/`.

## Setup

All operational scripts live in [`scripts/builds/`](scripts/builds/). Copy the directory to `/root/builds/` on the target server before running anything.

1. Place certificates in `/root/certs/` on the server (Cloudflare Origin cert for `*.vcmi.eu`, the Origin cert for `builds.vcmi.download`, and the Cloudflare authenticated-pull root).
2. `scp -r scripts/builds root@new-server:/root/`
3. On the server:
   ```sh
   cd /root/builds
   ./prepare.sh
   UPLOADER_PUBKEY='ssh-ed25519 AAAA... root@vcmi-web' \
   BUILDS_VOLUME_DEV=/dev/disk/by-id/scsi-0HC_Volume_<volume-id> \
       ./setup.sh
   mount /home/downloader     # if BUILDS_VOLUME_DEV was set
   ```

`UPLOADER_PUBKEY` must match the SSH private key stored as the corresponding Github secret in `vcmi/vcmi` (CI uses it to upload builds via sftp). Rotating the key is two operations: change `UPLOADER_PUBKEY` here, and update the Github secret.

`BUILDS_VOLUME_DEV` is optional — if set, an fstab line is appended. The volume must be attached at the provider control panel separately.

## Migration

If the new server can take over the existing builds volume (same provider, volume detach/reattach supported), move it across and re-run Setup on the new server (Setup is idempotent). The rest of this section covers moves where the volume can't follow.

Copying ~50 GB of static content over the network takes hours, but a single rsync handles it:

1. Provision the new server (per Setup above — `prepare.sh` + `setup.sh`). This creates the `downloader` user with the right uid so file ownership lands correctly after rsync.
2. Stop the cron job on the old server so files don't shift during the copy:
   ```sh
   ssh root@old-server 'crontab -l | grep -v on_cron_update.sh | crontab -'
   ```
3. From your workstation, with agent forwarding, run rsync directly between the two servers (the workstation's SSH agent authenticates both hops; the bulk traffic does not route through the workstation):
   ```sh
   ssh -A root@old-server \
       'rsync -a --delete /home/downloader/www/ root@new-server:/home/downloader/www/'
   ```
   `-a` preserves mtimes, which `ensure_free_space.sh` uses to pick the oldest builds for deletion. Without `-a` (or with a non-preserving tool), the auto-cleanup would mis-rank files on the new server.
4. Repeat the same rsync once just before the DNS cutover to pick up anything that landed during the long initial copy.
5. Switch the DNS records for `download.vcmi.eu`, `upload.vcmi.eu`, `builds.vcmi.download` to the new server.
6. Re-enable the old cron only if the old server stays as a fallback; otherwise leave it stopped.

## Troubleshooting

- Run `/opt/vcmiscripts/on_cron_update.sh` manually to check for possible issues.
- Ensure that `sudo -u downloader df /home/downloader/www/branch/` works and outputs size of volume with builds. If it results in "permission denied", make sure that all directories in that path have `+x` permission.
