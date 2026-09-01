#!/bin/bash

# Prevent overlapping runs: cron fires every minute, but ensure_free_space.sh
# can take longer than that under load. Without the lock, two copies would
# race over the same /home/uploader/uploads files.
exec 9>/var/lock/vcmi-uploads.lock
flock -n 9 || exit 0

cd /

find /home/uploader/uploads -type f -mmin +2 -exec chmod 644 {} \; -exec chown downloader:downloader {} \; -print0 | xargs -0 -I {} mv {} /home/downloader/tmp

sudo -u downloader /opt/vcmiscripts/ensure_free_space.sh
sudo -u downloader /opt/vcmiscripts/sort_builds.sh
