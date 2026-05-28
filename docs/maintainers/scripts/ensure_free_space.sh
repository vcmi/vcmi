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
    # Calculate disk usage percentage
    USAGE=$(df "$TARGET_DIR" | awk 'NR==2 {print int($5)}')
    echo "Current disk usage of $TARGET_DIR is: $USAGE%"

    # Check if usage is above threshold
    if [[ $USAGE -gt $USAGE_THRESHOLD ]]; then
        # Find the modification time of oldest file across all subdirectories
        OLDEST_MTIME=$(find "$TARGET_DIR" -type f ! -path "$EXCLUDE_DIR" -exec stat -c %Y {} \; | sort -n | head -1)

        if [[ -z "$OLDEST_MTIME" ]]; then
            echo "No files found to delete"
            break
        fi

        # Calculate age in days
        CURRENT_TIME=$(date +%s)
        AGE_DAYS=$(( (CURRENT_TIME - OLDEST_MTIME) / 86400 ))
        echo "Oldest file: $OLDEST_FILE (Age: $AGE_DAYS days)"

        # Delete all files with the same age in all subdirectories using mtime
        find "$TARGET_DIR" -type f ! -path "$EXCLUDE_DIR" -mtime "$AGE_DAYS" -delete
        echo "Deleted files with age $AGE_DAYS days"
    else
        echo "Disk usage is below threshold ($USAGE% < $USAGE_THRESHOLD%)"
        break
    fi

done
