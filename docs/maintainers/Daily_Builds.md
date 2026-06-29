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

- `/opt/vcmiscripts/` - contains scripts that are also available in `scripts` directory next to docs
- `/home/uploader/upload` - incoming directory for files uploaded from Github Actions
- `/home/downloader/tmp` - temporary storage for files that have finished upload but yet to be sorted. Generally this directory should be empty except for tiny moments of time while script is running OR if script fails to sort some builds such as builds for new platforms with unexpected file names
- `/home/downloader/www` - permanent storage for files available to download via nginx by players

Custom scripts (located in `/opt/vcmiscripts/` directory):

- `on_cron_update.sh` - runs every minute via cron as root and does following actions:
  - moves builds that finished upload from `/home/uploader/upload` to `/home/downloader/tmp`, changing file owners and permissions
  - executes scripts `ensure_free_space.sh` and `sort_builds.sh` as user `downloader`
- `ensure_free_space.sh` - automatically removes oldest builds from `/home/downloader/www/`, except for releases in `/home/downloader/www/branch/master`.
- `sort_builds.sh` - renames and moves builds from `/home/downloader/tmp` to various directories in `/home/downloader/www/`

## Setup

- Copy scripts from `scripts` directory to `/opt/vcmiscripts`
- Execute script `setup_uploader.sh`
- If needed, attach volume for builds storage via fstab: `/dev/disk/by-id/scsi-0DO_Volume_vcmi-second-builds /home/downloader ext4 defaults,nofail,discard 0 0`
- Ensure that `/home/downloader` and `/home/downloader/www` are owned by root, while all `/home/downloader/www` content is owned by `downloader` user
- Copy sites definitions from `scripts/sites` to `/etc/nginx/sites-available` and symlink them to `/etc/nginx/sites-enabled`
- Add certificates to nginx
- Install `fancy_index` nginx module: `apt install libnginx-mod-http-fancyindex`
- Restart nginx

## Migration

- Detach volume from old server on Digital Ocean control panel
- Attach volume to new server
- Setup new server as described in this document

## Troubleshooting

- Run script `/opt/vcmiscripts/on_cron_update.sh` manually to check for possible issues
- Ensure that command `sudo -u downloader df /home/downloader/www/branch/` works and outputs size of volume with builds. If it results in "permission denied", make sure that all directories in that path have `+x` permission
