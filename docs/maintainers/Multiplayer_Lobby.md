# Multiplayer lobby

- RAM requirements: 512 MB (may increase over time if lobby gets more active players)
- CPU requirements: 1 core (preferrably dedicated to ensure low latency)
- SSD requirements: up to 4 Gb (depending on log and database size over time)

Exposed to public as:

- `lobby.vcmi.eu:3031` - user login
- `beholder.vcmi.eu:3031` - old domain name for logins
- `api.vcmi.eu` - public API access (behind Cloudflare)  

- Start: `cd /home/lobby && nohup sudo -u lobby /usr/games/vcmilobby &`
- Stop: `killall -9 vcmilobby`
- Examine database (can be done live): `sqlite3 /home/lobby/.local/share/vcmi/vcmiLobby.db`
- Examine log file: `tail -n 100 /home/lobby/cache/vcmi/VCMI_Lobby_log.txt`

## Setup

Preparation:

- Generate .deb package with lobby binaries. Currently we have "Build VCMI Lobby" job in CI that does this. Produced .deb file needs to be uploaded to server
- Create dump of existing SQL database: `sqlite3 /home/lobby/local/share/vcmi/vcmiLobby.db ".backup 'vcmiLobbyBak.db'"`. Optionally it can be compressed via `gzip vcmiLobbyBak.db`
- Copy database dump on new server, and decompress it if needed via `gunzip vcmiLobbyBak.db.gz`

Once preparation is done, run the following commands:

```sh
apt install nginx sqlite3 sqlite3-tools
apt install ./vcmi-lobby.deb #NOTE: name may be different

useradd -m lobby
sudo -u lobby mkdir -p /home/lobby/.local/share/vcmi/

# to create new DB
sudo -u lobby sqlite3 ~/.local/share/vcmi/vcmiLobby.db </usr/share/vcmi/config/lobby.sql

# to restore existing DB
mv vcmiLobbyBak.db /home/lobby/.local/share/vcmi/vcmiLobby.db 
chown lobby:lobby /home/lobby/.local/share/vcmi/vcmiLobby.db 

# Start lobby
cd /home/lobby && nohup sudo -u lobby /usr/games/vcmilobby &
```

### Upgrade

- Generate and upload new .deb package
- Shut down old version of lobby: `killall -9 vcmilobby`
- Install new .deb package via `apt install ./vcmi-lobby.deb`
- Generate empty database with new schema: `sqlite3 "schema.db" < "/usr/share/vcmi/config/lobby.sql"`
- Compare old and new databases `sqldiff --schema vcmiLobby.db schema.db > upgrade.sql`
- Open upgrade script and edit it if necessary: `nano upgrade.sql`
- Make sure that you have database backup: `sqlite3 /home/lobby/local/share/vcmi/vcmiLobby.db ".backup 'vcmiLobbyBak.db"`
- Upgrade database to new schema: `sqlite3 vcmiLobby.db < upgrade.sql`
- Re-run compare to verify that databases are now the same: `sqldiff --schema vcmiLobby.db schema.db > upgrade.sql`
- Start new lobby server `cd /home/lobby && nohup sudo -u lobby /usr/games/vcmilobby &`

#### Troubleshooting

- Lobby crashes on start due to `boost::filesystem::status: Permission denied [system:13]: "config"`. Solution: `cd /home/lobby` (or any other directory writeable by `lobby` user)
- Lobby shut downs after logout from server: ensure that `vcmilobby` was started via `nohup`
