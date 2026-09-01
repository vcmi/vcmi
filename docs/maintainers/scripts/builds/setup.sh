#!/bin/bash
# Create uploader/downloader users, install HEADER/FOOTER and the cron job.
# Requires (from env):
#   UPLOADER_PUBKEY    — full "ssh-ed25519 AAAA... root@vcmi-web" line.
#                        Must match the private key stored as GitHub secret
#                        in vcmi/vcmi (used by CI to upload builds via sftp).
# Optional:
#   BUILDS_VOLUME_DEV  — /dev/disk/by-id/... for the builds storage volume.
#                        If set, it is added to fstab and mounted before the
#                        served directories are created. Attach it first.
set -euo pipefail

: "${UPLOADER_PUBKEY:?required}"

HERE="$(dirname "$0")"

# --- uploader: sftp-only drop folder ---
id -u uploader >/dev/null 2>&1 || useradd -m -s /usr/sbin/nologin uploader
install -d -m 755 -o root      -g root      /home/uploader
install -d -m 333 -o uploader  -g uploader  /home/uploader/uploads
install -d -m 700 -o uploader  -g uploader  /home/uploader/.ssh
echo "$UPLOADER_PUBKEY" >/home/uploader/.ssh/authorized_keys
chown uploader:uploader /home/uploader/.ssh/authorized_keys
chmod 600 /home/uploader/.ssh/authorized_keys

# --- downloader: owns the served files ---
id -u downloader >/dev/null 2>&1 || useradd -m -s /bin/false downloader

# Mount the builds volume before creating the served tree: otherwise the
# directories are created on the root filesystem and then hidden by the mount.
if [[ -n "${BUILDS_VOLUME_DEV:-}" ]]; then
    FSTAB_LINE="$BUILDS_VOLUME_DEV /home/downloader ext4 defaults,nofail,discard 0 0"
    grep -qF "$BUILDS_VOLUME_DEV" /etc/fstab || echo "$FSTAB_LINE" >>/etc/fstab
    mountpoint -q /home/downloader || mount /home/downloader
fi

install -d -m 755 -o downloader -g downloader /home/downloader/tmp
install -d -m 755 -o downloader -g downloader /home/downloader/www

# nginx fancyindex header/footer must be readable inside the www root.
install -m 644 "$HERE/HEADER.html" /home/downloader/www/
install -m 644 "$HERE/FOOTER.html" /home/downloader/www/

# --- cron ---
CRON_LINE="* * * * * /opt/vcmiscripts/on_cron_update.sh"
( crontab -l 2>/dev/null | grep -Fv on_cron_update.sh || true; echo "$CRON_LINE" ) | crontab -

systemctl reload nginx
