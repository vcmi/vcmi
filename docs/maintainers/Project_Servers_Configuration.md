< [Documentation](../Readme.md) / Project Servers Configuration


This page dedicated to explain specific configurations of our servers for anyone who might need to improve it in future. Check [project infrastructure](project_infrastructure "wikilink") page for services and accounts overview.

## Droplet configuration

### Droplet and hosted services

Currently we using two droplets:

- First one serve all of our web services:
 - [Forum](https://forum.vcmi.eu/)
 - [Bug tracker](https://bugs.vcmi.eu/)
 - [Wiki](https://wiki.vcmi.eu/)
 - [Slack invite page](https://slack.vcmi.eu/)
- Second serve downloads:
 - [Legacy download page](http://download.vcmi.eu/)
 - [Build download page](https://builds.vcmi.download/)

To keep everything secure we should always keep binary downloads separate from any web services.

### Rules to stick to

- SSH authentication by public key only.
- Incoming connections to all ports except SSH (22) must be blocked.
- Exception for HTTP(S) connection on ports 80 / 443 from [CloudFlare IP Ranges](https://www.cloudflare.com/ips/).
- No one except core developers should ever know real server IPs.
- Droplet hostname should never be valid host. Otherwise it's exposed in [reverse DNS](https://en.wikipedia.org/wiki/Reverse_DNS).
- If some non-web service need to listen for external connections then read below.

### Our publicly-facing server

We only expose floating IP that can be detached from droplet in case of emergency using [DO control panel](https://cloud.digitalocean.com/networking/floating_ips). This also allow us to easily move public services to dedicated droplet in future.

- Address: beholder.vcmi.eu (67.207.75.182)
- Port 22 serve SFTP for file uploads as well as CI artifacts uploads.

If new services added firewall rules can be adjusted in [DO control panel](https://cloud.digitalocean.com/networking/firewalls).