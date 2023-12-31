< [Documentation](../Readme.md) / Project Infrastructure

# Project Infrastructure

This section hold important information about project infrastructure for current and future contributors. At moment it's all maintained by me (SXX), but following information will be useful if someone going to replace me in future.

## Services and accounts

So far we using following services:

### Most important

- VCMI.eu domain paid until July of 2019.
 - Owner: Tow
 - Our main domain used by services.
- VCMI.download paid until November of 2026.
 - Owner: SXX
 - Intended to be used for all assets downloads.
 - Domain registered on GANDI and **can be renewed by anyone without access to account**.
- [DigitalOcean](https://cloud.digitalocean.com/) team.
 - Our hosting sponsor.
 - Administrator access: SXX, Warmonger.
 - User access: AVS, Tow.
- [CloudFlare](https://www.cloudflare.com/a/overview) account.
 - Access through shared login / password.
 - All of our infrastructure is behind CloudFlare and all our web. We manage our DNS there.
- [Google Apps (G Suite)](https://admin.google.com/) account.
 - It's only for vcmi.eu domain and limited to 5 users. Each account has limit of 500 emails / day.
 - One administrative email used for other services registration.
 - "noreply" email used for outgoing mail on Wiki and Bug Tracker.
 - "forum" email used for outgoing mail on Forums. Since we authenticate everyone through forum it's should be separate email.
 - Administrator access: Tow, SXX.
- [Google Play Console](https://play.google.com/apps/publish/) account.
 - Hold ownership over VCMI Android App.
 - Owner: SXX
 - Administrator access: Warmonger, AVS, Ivan.
 - Release manager access: Fay.

Not all services let us safely share login credentials, but at least when possible at least two of core developers must have access to them in case of emergency.

### Public relations

We want to notify players about updates on as many social services as possible.

- Facebook page: <https://www.facebook.com/VCMIOfficial>
 - Administrator access: SXX, Warmonger
- Twitter account: <https://twitter.com/VCMIOfficial>
 - Administrator access: SXX.
 - User access via TweetDeck:
- VK / VKontakte page: <https://vk.com/VCMIOfficial>
 - Owner: SXX
 - Administrator access: AVS
- Steam group: <https://steamcommunity.com/groups/VCMI>
 - Administrator access: SXX
 - Moderator access: Dydzio
- Reddit: <https://reddit.com/r/vcmi/>
 - Administrator access: SXX
- ModDB entry: <http://www.moddb.com/engines/vcmi>
 - Administrator access: SXX

### Communication channels

- Slack team: <https://h3vcmi.slack.com/>
  - Owner: vmarkovtsev
  - Administrator access: SXX, Warmonger, AVS...
- Trello team: <https://trello.com/vcmi/>
  - Administrator access: SXX
- Discord:
  - Owner: dydzio
  - Administrator access: SXX, Warmonger, Ivan...

### Other services

- Launchpad PPA: <https://launchpad.net/~vcmi>
 - Member access: AVS
 - Administrator access: Ivan, SXX
- Snapcraft Dashboard: <https://dashboard.snapcraft.io/>
 - Administrator access: SXX
- Coverity Scan page: <https://scan.coverity.com/projects/vcmi>
 - Administrator access: SXX, Warmonger, AVS
- OpenHub page: <https://www.openhub.net/p/VCMI>
 - Administrator access: Tow
- Docker Hub organization: <https://hub.docker.com/u/vcmi/>
 - Administrator access: SXX

Reserve accounts for other code hosting services:

- GitLab organization: <https://gitlab.com/vcmi/>
 - Administrator access: SXX
- BitBucket organization: <https://bitbucket.org/vcmi/>
 - Administrator access: SXX

## What's to improve

1.  Encourage Tow to transfer VCMI.eu to GANDI so it's can be also renewed without access.
2.  Use 2FA on CloudFlare and just ask everyone to get FreeOTP and then use shared secret.
3.  Centralized way to post news about game updates to all social media.

# Project Servers Configuration

This section dedicated to explain specific configurations of our servers for anyone who might need to improve it in future.

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