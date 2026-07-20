#!/bin/bash

# Ensures that used space on drive that contains this directory stays below 90%
# Oldest builds will be automatically removed to make space for new uploads

# Base directory that needs free space
TARGET_DIR="/home/downloader/www/branch"
# Exlude master builds (releases) from auto-removal
EXCLUDE_DIR="/home/downloader/www/branch/master/*"
# Arbitrary threshold, can be adjusted
USAGE_THRESHOLD=90

while true; do
    USAGE=$(df "$TARGET_DIR" | awk 'NR==2 {print int($5)}')
    echo "Current disk usage of $TARGET_DIR is: $USAGE%"

    if [[ $USAGE -le $USAGE_THRESHOLD ]]; then
        echo "Disk usage is below threshold ($USAGE% <= $USAGE_THRESHOLD%)"
        break
    fi

    # Pick the single oldest non-excluded file and delete it. NUL-delimited so
    # paths with spaces / newlines are safe.
    OLDEST=$(find "$TARGET_DIR" -type f ! -path "$EXCLUDE_DIR" -printf '%T@\t%p\0' \
             | sort -zn | head -zn 1 | cut -z -f2-)

    if [[ -z "$OLDEST" ]]; then
        echo "No deletable files remain; usage still $USAGE%"
        break
    fi

    echo "Deleting: $OLDEST"
    rm -f -- "$OLDEST" || break
done
