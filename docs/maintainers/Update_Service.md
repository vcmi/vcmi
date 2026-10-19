# Update service

- RAM requirements: under 100 MB
- CPU requirements: non-existing
- SSD requirements: non-existing; reads builds from the [downloads server](Daily_Builds.md), which must be on the same host

Exposed to public as:

- `updates.vcmi.eu/autoupdate.json` - update information for VCMI launchers (behind Cloudflare; nginx proxies HTTPS to the service's plain-HTTP listener on `127.0.0.1:3040`)
- `updates.vcmi.eu/altstore.json` - [AltStore source](https://faq.altstore.io/developers/make-a-source) for side-loaded iOS builds, also used by SideStore and similar apps

The code is in [`updates/`](../../updates/) and is packaged as a `.deb` by the "Build VCMI Update Service" CI job.

- Start / stop / restart: `systemctl start|stop|restart vcmi-updates`
- Examine log: `journalctl -u vcmi-updates`
- Check all data and the build files that can be offered: `vcmi-updates-release check`
- Configuration: `/etc/vcmi-updates.conf`
- Request statistics: `/var/log/nginx/updates.access.log` holds time, version, OS, architecture, distribution and channel of every request, without client addresses. Kept for 14 days by the default nginx logrotate configuration. Requests per day and version: `awk '{print substr($1, 1, 10), $2}' /var/log/nginx/updates.access.log | sort | uniq -c`

## Data

The service reloads its data within 10 seconds of any change. If changed data is invalid, the service logs an error and keeps using the previous data. Run `vcmi-updates-release check` after every hand edit.

- `/var/lib/vcmi-updates/releases/<version>.json` - release record, created by `vcmi-updates-release add`
- `/var/lib/vcmi-updates/releases/<version>.md` - changelog section shown for that release
- `/var/lib/vcmi-updates/channels/<channel>.json` - source of updates for the `stable`, `beta` and `develop` channels
- `/var/lib/vcmi-updates/altstore.json` - template of the AltStore source
- `/home/downloader/www/branch/<branch>/metadata/*.json` - build manifests uploaded by CI, one per CI run, listing all builds of that run

### Release record

- `version` - release version, `1.X.Y`. Must match the file name and the build manifest
- `commit` - 7-character commit of the `master` build; selects `branch/master/metadata/VCMI-branch-master-<commit>.json`
- `date` - release date shown to players, `YYYY-MM-DD`. Set by `vcmi-updates-release publish`
- `status` - `draft` or `published`. Only `published` records are offered
- `partial` - optional, `true` if only platforms listed in `platforms` receive this release
- `common` - optional section that applies to all platforms
- `platforms` - optional sections for an OS (`windows`) or a platform (`windows-x64`). Platform keys use the `os` and `arch` values sent by the launcher

Each section may have:

- `action` - highest action allowed for these platforms: `install`, `download` or `external`. Default is `install`
- `notes` - list of short messages shown to players above the changelog

Sections are applied in the order common, OS, platform. `notes` of all three are shown.

```json
{
	"version": "1.8.1",
	"commit": "6901839",
	"date": "2026-11-20",
	"status": "published",
	"common": {
		"notes": [ "Saved games from 1.7 can be loaded in 1.8" ]
	},
	"platforms": {
		"windows-arm64": { "action": "download" }
	}
}
```

### Channels

- `{ "source": "releases" }` - offer the latest published release
- `{ "source": "branch", "branch": "beta" }` - offer the latest build of this branch

`stable` always uses `releases`. When a beta phase is over, copy `stable.json` over `beta.json`: beta builds are then offered the stable release. When a new beta phase starts, restore `{ "source": "branch", "branch": "beta" }`.

### AltStore source

`/altstore.json` is the template with an entry for every published release that applies to `ios-arm64` and whose `ios-arm64` package matches the build manifest. The entries of releases are placed before the entries of the template, highest version first:

- `version`, `date` - from the release record
- `size`, `downloadURL` - of the `ios-arm64` package in the build manifest of the release, on download.vcmi.eu
- `minOSVersion` - `min_os_version` in the `[altstore]` section of the configuration
- `localizedDescription` - `notes` of the release record for `ios-arm64`, followed by the changelog section without its heading

The template holds everything else: the source and app metadata, screenshots, and entries of releases made before the update service, which download from GitHub releases. It must contain exactly one app. A version that has a release record must not be listed in the template. The service is the only source of `/altstore.json`, so screenshots and icons are referenced by URL from other servers.

`setup.sh` creates the template from [`scripts/updates/altstore.json`](scripts/updates/altstore.json) if it does not exist yet. Later changes are made on the server.

## Making a release

1. Merge `beta` into `master` and wait for the CI run to finish, including the job that uploads the build manifest
2. Make sure the release section in `ChangeLog.md` on `master` is final (no `(in development)`)
3. `vcmi-updates-release add 1.X.Y <commit> --notes "..."` - checks the build manifest and its files, fetches the changelog section, and creates a draft record
4. Optionally, edit the draft by hand, then run `vcmi-updates-release check`
5. Publish the GitHub release
6. `vcmi-updates-release publish 1.X.Y`
7. Verify: `curl 'https://updates.vcmi.eu/autoupdate.json?version=1.0.0&os=windows&arch=x64&dist=installer'` and `curl https://updates.vcmi.eu/altstore.json`

Fix for some platforms only: the same, with a new version number and `--only <OS or platform>...`.

To withdraw a release, set its `status` back to `draft` and run `vcmi-updates-release check`. Clients are offered the previous published release instead.

## Setup

All operational scripts live in [`scripts/updates/`](scripts/updates/). Copy the directory to `/root/updates/` on the host of the downloads server.

1. Add DNS record `updates.vcmi.eu` for the host in Cloudflare, proxied
2. Place certificates in `/root/certs/` on the server (`dot.vcmi.eu.pem`, `dot.vcmi.eu.key`, `cloudflare-client.crt`)
3. Place the `.deb` package produced by the "Build VCMI Update Service" CI job at any reachable path, e.g. `/root/vcmi-updates.deb`
4. `scp -r scripts/updates root@new-server:/root/`
5. On the server:
   ```sh
   cd /root/updates
   ./prepare.sh
   ./setup.sh /root/vcmi-updates.deb
   ```

`setup.sh` creates channel configurations and the AltStore template if they do not exist yet.

## Upgrade

```sh
./upgrade.sh /root/vcmi-updates-new.deb
```

Data in `/var/lib/vcmi-updates` is not part of the package and is kept.

## Migration

The service must move together with the downloads server, since it reads build files from disk.

1. On the old server: `./export.sh` - produces `/root/updates-backup.tar.gz`
2. Provision the new server and run the Setup steps above
3. Transfer the backup: `ssh -A root@old-server "scp /root/updates-backup.tar.gz root@new-server:/root/"`
4. On the new server: `./import.sh /root/updates-backup.tar.gz`
