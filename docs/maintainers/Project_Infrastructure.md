# Project Infrastructure

## What's to improve

1. Encourage Tow to transfer VCMI.eu to GANDI so it's can be also renewed without access.
2. Centralized way to post news about game updates to all social media.
3. Verify VCMI.eu domain name expiration date with Tow
4. Verify VCMI.download domain name expiration date with SXX
5. Verify Google Apps (G Suite) status with Tow
6. Restore firewall which for some reason is disabled on DO

## Services and accounts

### Infrastructure

| Service | Details | Owner | Administrators | Notes |
| ------- | ------- | ----- | -------------- | ----- |
| [GitHub](https://github.com/vcmi/) | Code hosting, bug tracker, pull requests, website hosting | - | Tow, AVS, Ivan, Warmonger, SXX | - |
| [VCMI.eu domain name](https://vcmi.eu) | Main domain for services | Tow | - | Renewal date **unknown** |
| [VCMI.download domain name](https://vcmi.download) | Secondary domain name for downloads | SXX | - | Paid until **November 2026**. Registered on GANDI; **can be renewed by anyone without account access** |
| [DigitalOcean](https://cloud.digitalocean.com/) | Hosting sponsor for all our self-hosted services | - | SXX, Warmonger, Ivan, AVS, Tow | - |
| [CloudFlare](https://www.cloudflare.com/a/overview) | DNS & CDN for our web services | - | SXX, Ivan | All our web services are behind CloudFlare and use Cloudflare SSL certificates |
| [Weblate](https://hosted.weblate.org/projects/vcmi/) | Game translations | - | Ivan | Hosts translations for VCMI itself (not including mods & website). Uses free "Gratis" plan |
| [Google Play Console](https://play.google.com/apps/publish/) | VCMI Android App | SXX | Warmonger, AVS, Ivan, Fay | - |
| [Google Apps (G Suite)](https://admin.google.com/) | Email for vcmi.eu domain | - | Tow, SXX | Limited to 5 users; 500 emails/day limit per account. Includes: admin email (service registration), "noreply" (Wiki/Bug Tracker), "forum" (Forums authentication). **Likely dead. Verify with Tow** |
| [Launchpad PPA](https://launchpad.net/~vcmi) | Ubuntu package repository | Mantas Kriaučiūnas | Ivan, SXX, AVS | Contains daily builds and latest releases PPA's for Ubuntu |
| [Sonar Cloud](https://sonarcloud.io/project/overview?id=vcmi_vcmi) | Code analysis | - | Shares credentials with Github | Integrated into Github pull requests |
| [Discord Team](https://discord.com/developers/teams/1467529815450976543/information/) | Discord app holder | Ivan | Laserlicht, dydzio, Warmonger | Holds ownership of [Discord VCMI app](https://discord.com/developers/applications/1416538363254669455/information) that is used to display rich presence when people are playing VCMI |
| [Snapcraft Dashboard](https://dashboard.snapcraft.io/) | Snap package distribution | - | SXX | Abandoned in favor of Flatpaks and PPA |
| [Coverity Scan](https://scan.coverity.com/projects/vcmi) | Code analysis | - | SXX, Warmonger, AVS | Abandoned in favor of Sonar Cloud |
| [OpenHub](https://www.openhub.net/p/VCMI) | Code statistics | - | Tow | - |
| [Docker Hub](https://hub.docker.com/u/vcmi/) | Container registry | - | SXX | Abandoned and never used? |
| [GitLab](https://gitlab.com/vcmi/) | Code repository | - | SXX | Reserve account, not used |
| [BitBucket](https://bitbucket.org/vcmi/) | Code repository | - | SXX | Reserve account, not used |

Note: "Owner" refers to services that require one (and only one) account to have special superuser-like status, potentially - with legal and/or biling information. If service has no such requirement, this field is left blanc.

When possible at least two of active core developers must have access to them in case of emergency.

#### Communities with page managed by VCMI Team

| Service Name | Owner | Administrators | Notes |
| ------------ | ----- | -------------- | ----- |
| [Discord](https://discord.com/) | dydzio | SXX, Warmonger, Ivan... | Main communication platform |
| [Facebook page](https://www.facebook.com/VCMIOfficial) | — | SXX, Warmonger | Active |
| [Reddit](https://reddit.com/r/vcmi/) | — | SXX | Abandoned in favor of general H3 subreddits |
| [Twitter account](https://twitter.com/VCMIOfficial) | — | SXX | Abandoned, User access via TweetDeck |
| [VK / VKontakte page](https://vk.com/VCMIOfficial) | SXX | AVS | Abandoned |
| [Steam group](https://steamcommunity.com/groups/VCMI) | SXX | Dydzio | Abandoned |
| [ModDB entry](http://www.moddb.com/engines/vcmi) | — | SXX | Abandoned |
| [Slack team](https://h3vcmi.slack.com/) | vmarkovtsev | SXX, Warmonger, AVS... | Abandoned in favor of Discord |
| [Trello team](https://trello.com/vcmi/) | — | SXX | Abandoned |

#### Heroes 3 communities with VCMI Team presence

| Service Name | Active team members | Notes |
| ------------ | ------------------- | ----- |
| [VCMI thread on Heroes Community](http://heroescommunity.com/viewthread.php3?TID=30659&pagenumber=1) | Warmonger, Ivan, dydzio... | Very low player activity |
| [Heroes 3 subreddit](https://www.reddit.com/r/heroes3/) | Ivan, dydzio... | VCMI-related questions are rather common |
| [HoMM subreddit](https://www.reddit.com/r/HoMM/) | Ivan, dydzio... | Way less active than Heroes 3 subreddit, but sometimes posts about VCMI do appear |

## Project Servers Configuration

This section dedicated to explain specific configurations of our servers for anyone who might need to improve it in future.

### VPS configuration

At the moment, most our services are hosted by Digital Ocean. Current approach is to keep services on separate VPS (called "droplets" by Digital Ocean) for better isolation & to allow independent restarts / upgrades. This also allows us to measure performance & system load of each service independently. All droplets can only be accessed using ssh login with public key. Currently access to all droplets is granted to:

- Ivan Savenko
- Alexvins
- Warmonger
- Tow
- SXX
- kambala (`vcmi-artifactory` droplet)

Lobby is currently hosted on Hetzner, with migration of other services to Hetzner in plans. Login is via public key, currently granted to:

- Ivan Savenko

| VPS | Location | Specifications | Services |
| --- | -------- | -------------- | -------- |
| `vcmi-forum` | DO Droplet | 2 Gb / 1 CPU / 25 Gb / $12 (+20%) | [Discourse forum](https://forum.vcmi.eu/). Note: 25 Gb droplet - ssd can be expanded, or we can downscale entire droplet to 1 Gb config |
| `vcmi-weblate` | DO Droplet | 2 Gb / 1 CPU / 50 Gb / $12 | [Weblate](https://weblate.vcmi.eu/) |
| `vcmi-web` | DO Droplet | 512 Mb / 1 CPU / 10 Gb + 100 Gb / $4 (+20%) + $10 | Builds uploading from Github, [Build download page](http://download.vcmi.eu/), [Legacy download page](https://builds.vcmi.download/). Also contains nginx server for redirecting [old bug tracker](https://bugs.vcmi.eu/), [old wiki](https://wiki.vcmi.eu/), and [old slack invite page](https://slack.vcmi.eu/) |
| `vcmi-lobby` | Hetzner Server | 4 Gb / 2 CPU / 40 Gb / €4 (+20%) | Multiplayer lobby (lobby.vcmi.eu or beholder.vcmi.eu - deprecated) as we ll as [API endpoint](https://api.vcmi.eu/) |
| `vcmi-artifactory` | DO Snapshot | 4 Gb / 2 CPU / 80 Gb / $24 | [Conan Artifactory server](https://artifactory.vcmi.eu/) |
| `vcmi-main` | DO Snapshot | ??? / $1 | Contains old bugtracker, forum, and wiki |
| `vcmi-second` | DO Snapshot | ??? / $1 | Contains old MP lobby and builds uploader |

Notes:

- All active VPS run Ubuntu 24.04
- VPS with deployed and tested services have backups enabled (+20% costs)
- In addition, we have separate 100 Gb volume for builds ($10 / month), currently attached to `vcmi-web`

### Rules to stick to

- SSH authentication by public key only.
- Incoming connections to all ports except SSH (22) must be blocked.
- Exception for HTTP(S) connection on ports 80 / 443 from [CloudFlare IP Ranges](https://www.cloudflare.com/ips/).
- No one except core developers should ever know real server IPs.
- Droplet hostname should never be valid host. Otherwise it's exposed in [reverse DNS](https://en.wikipedia.org/wiki/Reverse_DNS).
- If some non-web service need to listen for external connections, it needs to use "Reserve IP" for it. If new services added firewall rules can be adjusted in [DO control panel](https://cloud.digitalocean.com/networking/firewalls).

## Domain names

| Domain | Content | Hosted on | Notes |
| ------ | ------- | --------- | ----- |
| [vcmi.eu](https://vcmi.eu) | Main page redirect | CNAME | No content, redirects to [real main page](https://vcmi.github.io/) |
| [download.vcmi.eu](https://download.vcmi.eu) | Public downloads & daily builds | `vcmi-web` | - |
| [upload.vcmi.eu](https://upload.vcmi.eu) | Domain name for uploading daily builds from Github | `vcmi-web` | No http services |
| [beholder.vcmi.eu](https://beholder.vcmi.eu) | Multiplayer lobby | `vcmi-lobby` | No http services. Used for VCMI 1.7.3 lobby and older. Deprecated in favor of `lobby` |
| [lobby.vcmi.eu](https://lobby.vcmi.eu) | Multiplayer lobby | `vcmi-lobby` | No http services |
| [api.vcmi.eu](https://lobby.vcmi.eu) | Multiplayer lobby API endpoint | `vcmi-lobby` | nginx acts as proxy to provide https wrapper for http-only REST API |
| [forum.vcmi.eu](https://forum.vcmi.eu) | Discourse forum | `vcmi-forum` | - |
| [bugs.vcmi.eu](https://bugs.vcmi.eu) | Bug tracker | `vcmi-web` | Redirects to [Github Issues](https://github.com/vcmi/vcmi/issues) |
| [slack.vcmi.eu](https://slack.vcmi.eu) | Slack invite page | `vcmi-web` | Redirects to [main page](https://vcmi.eu/) |
| [weblate.vcmi.eu](https://weblate.vcmi.eu) | Weblate translation service | `vcmi-weblate` | - |
| [wiki.vcmi.eu](https://wiki.vcmi.eu) | Wiki | `vcmi-web` | Redirects to [main page](https://vcmi.eu/) |
| [vcmi.download](https://vcmi.download) | Main page redirect | CNAME | No content, redirects to [main page](https://vcmi.eu/) |
| [builds.vcmi.download](https://builds.vcmi.download) | Public downloads | `vcmi-second` | Redirects to [download.vcmi.eu](https://download.vcmi.eu) |

## Self-hosted services

Currenly we have following services deployed:

- [Discourse](Discourse.md)
- [Multiplayer lobby](Multiplayer_Lobby.md)
- [Downloads & daily builds](Daily_Builds.md)
- [Weblate](Weblate.md)

Potential addition for the future:

- Conan Artifactory
- Crash reporter tool, such as [GlitchTip](https://glitchtip.com/)
- (long-term) Expanded multiplayer lobby with cheat-proof game hosting

### Web Hosting

For all web services we use Nginx, including websites that run on standalone servers. This allows to easily migrate more services onto another server that already has some services, and use nginx reverse proxy to host all such services on same 443 port.

For certificates all services use Cloudflare Origin certificates. These certificates are issued for free in Cloudflare web UI, have expiration period of 10 years (oldest VCMI certificate will expire in 2032), but can only be used by services that are located behind Cloudflare, which all VCMI services do. Client-facing certificates are managed and automatically updated by Cloudflare.

### Configuration files

See scripts directory that contains most of customized configuration files used on our servers. For obvious reasons, sensitive parts that include password and other non-public data are excluded for it
