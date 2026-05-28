# Weblate

- RAM requirements: 4 GB, may run at 2 Gb, but performance is poor
- CPU requirements: 2 core, may tun at 1 core but performance is poor
- SSD requirements: over 10 GB. Will grow over time with more translations

Accessible as [weblate.vcmi.eu](https://weblate.vcmi.eu/)

Admin access:

- Ivan Savenko

Additional accounts can be given admin rights via admin panel.

## Configuration

- Located at `/var/weblate-docker`.
- Configuration file is at `/var/weblate-docker/docker-compose.override.yml`
- Administration primarily via web interface
- To apply configuration changes and update weblate:
  ```sh
  # Fetch latest versions of the images
  docker-compose pull
  # Stop and destroy the containers
  docker-compose down
  # Spawn new containers in the background
  docker-compose up -d
  # Follow the logs during upgrade
  docker-compose logs -f
  ```

## Setup

See [Official docs](https://docs.weblate.org/en/latest/admin/install/docker.html):

Approximate order of commands:

```sh
# Install git and docker
apt install docker.io git docker-compose nginx

# Setup nginx
cd /etc/nginx/sites-available/
nano weblate.vcmi.eu
cd ../sites-enabled/
ln -s ../sites-available/weblate.vcmi.eu 
service nginx restart 

# Setup Weblate
cd /var
git clone https://github.com/WeblateOrg/docker-compose.git weblate-docker
cd weblate-docker/
nano docker-compose.override.yml
docker compose up -d

# Optionally, create swap
fallocate -l 2G /swapfile
chmod 600 /swapfile
mkswap /swapfile
swapon /swapfile
nano /etc/fstab # append `/swapfile swap swap defaults 0 0` to the end, on new line
```

Modified configuration file. Check existing instance or other services (e.g. Discourse) for private fields:

```text
services:
  weblate:
    image: weblate/weblate:latest
    environment:
      WEBLATE_EMAIL_HOST: ***
      WEBLATE_EMAIL_HOST_USER: ***
      WEBLATE_EMAIL_HOST_PASSWORD: ***
      WEBLATE_SERVER_EMAIL: weblate@vcmi.eu
      WEBLATE_DEFAULT_FROM_EMAIL: weblate@vcmi.eu
      WEBLATE_SITE_DOMAIN: weblate.vcmi.eu
      WEBLATE_ADMIN_EMAIL: ***
      CELERY_SINGLE_PROCESS: 1
      WEBLATE_ENABLE_HTTPS: 1
      WEBLATE_IP_PROXY_HEADER: HTTP_X_FORWARDED_FOR
      WEBLATE_SECURE_PROXY_SSL_HEADER: HTTP_X_FORWARDED_PROTO,https
    ports:
    - "127.0.0.1:8080:8080"
```

## Upgrade

(not investigated)

## Migration

(not investigated)
