# Discourse

- RAM requirements: ~1.5 GB.
- CPU requirements: 1 core
- SSD requirements: ~20 GB. May be more over time

Accessible as [forum.vcmi.eu](https://forum.vcmi.eu/)

Admin access:

- Ivan Savenko
- AVS
- SXX
- Warmonger

Additional accounts can be given admin rights via admin panel. Or, if none are available - directly via server console (requires root access to server)

## Configuration

- Located at `/var/discourse`.
- Configuration file is at `/var/discourse/containers/app.yml`
- For administration typical approach is `cd /var/discourse; ./launcher enter app`. However most commands are also available in web UI
- To apply configuration changes, `cd /var/discourse; ./launcher rebuild app`. WARNING: this will also perform update of Discourse itself!

## Setup

References:

- [official docs](https://github.com/discourse/discourse/blob/main/docs/INSTALL-cloud.md):
- [nginx multi-server configuration](https://meta.discourse.org/t/run-other-websites-on-the-same-machine-as-discourse/17247)

```sh
wget -qO- https://raw.githubusercontent.com/discourse/discourse_docker/main/install-discourse | sudo bash
```

Configure Discourse to run through nginx outside of container, to allow multiple sites to be hosted on same server.

1. Install nginx
2. Edit configuration at `/var/discourse/containers/app.yml`:
3. Configure nginx server
4. Rebuild app and restart nginx

Modified configuration file:

```text
  - "templates/postgres.template.yml"
  - "templates/redis.template.yml"
  - "templates/web.template.yml"
  - "templates/web.ratelimited.template.yml"
  - "templates/web.socketed.template.yml"  # <-- Added
#   - "templates/web.ssl.template.yml" # remove - https will be handled by outer nginx
#   - "templates/web.letsencrypt.ssl.template.yml" # remove -- https will be handled by outer nginx
# expose: # comment out entire section by putting a # in front of each line
# - "80:80"   # http
# - "443:443" # https  
```

Approximate order of commands:

```sh
cd /var/discourse
apt install nginx
nano containers/app.yml

cd /etc/nginx/sites-available
cp default forum.vcmi.eu
cd ../sites-enabled
ln -s ../sites-available/forum.vcmi.eu

./launcher rebuild app
sudo service nginx reload
```

## Upgrade

```sh
cd /var/discourse
./launcher rebuild app
```

Alternatively can be done in UI, but untested

## Migration

When setting up Discourse, it needs to be already located on target domain name, so it might be a good idea to do operations in following order:

- create backup on old server (or pick latest weekly backup)
- switch domain name to new IP & wait for DNS to propagate
- ensure that ports 80 and 443 are open on new server
- setup Discourse on new server
- restore backup either in web interface or in console on new server
- adjust or migrate configuration file from old server to new one
