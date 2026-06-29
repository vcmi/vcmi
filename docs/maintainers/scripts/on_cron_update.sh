#!/bin/bash

find /home/uploader/uploads -type f -mmin +2 -exec chmod 644 {} \; -exec chown downloader:downloader {} \; -print0 | xargs -0 -I {} mv {} /home/downloader/tmp

sudo -u downloader ensure_free_space.sh
sudo -u downloader sort_builds.sh
