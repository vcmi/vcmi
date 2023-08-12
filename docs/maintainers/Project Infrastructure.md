This page hold important information about project infrastructure for
current and future contributors. At moment it's all maintained by me
(SXX), but following information will be useful if someone going to
replace me in future.

You can also check [detailed information on server
configuration](Project_servers_configuration "wikilink").

## Services and accounts

So far we using following services:

### Most important

-   VCMI.eu domain paid until July of 2019.
    -   Owner: Tow
    -   Our main domain used by services.
-   VCMI.download paid until November of 2026.
    -   Owner: SXX
    -   Intended to be used for all assets downloads.
    -   Domain registered on GANDI and **can be renewed by anyone
        without access to account**.
-   [DigitalOcean](https://cloud.digitalocean.com/) team.
    -   Our hosting sponsor.
    -   Administrator access: SXX, Warmonger.
    -   User access: AVS, Tow.
-   [CloudFlare](https://www.cloudflare.com/a/overview) account.
    -   Access through shared login / password.
    -   All of our infrastructure is behind CloudFlare and all our
        webWe'll manage our DNS there.
-   [Google Apps (G Suite)](https://admin.google.com/) account.
    -   It's only for vcmi.eu domain and limited to 5 users. Each
        account has limit of 500 emails / day.
    -   One administrative email used for other services registration.
    -   "noreply" email used for outgoing mail on Wiki and Bug Tracker.
    -   "forum" email used for outgoing mail on Forums. Since we
        authenticate everyone through forum it's should be separate
        email.
    -   Administrator access: Tow, SXX.
-   [Google Play Console](https://play.google.com/apps/publish/)
    account.
    -   Hold ownership over VCMI Android App.
    -   Owner: SXX
    -   Administrator access: Warmonger, AVS.
    -   Release manager access: Fay.

Not all services let us safely share login credentials, but at least
when possible at least two of core developers must have access to them
in case of emergency.

### Public relations

We want to notify players about updates on as many social services as
possible.

-   Facebook page: <https://www.facebook.com/VCMIOfficial>
    -   Administrator access: SXX, Warmonger
-   Twitter account: <https://twitter.com/VCMIOfficial>
    -   Administrator access: SXX.
    -   User access via TweetDeck:
-   VK / VKontakte page: <https://vk.com/VCMIOfficial>
    -   Owner: SXX
    -   Administrator access: AVS
-   Google+ page: <https://plus.google.com/+VCMIOfficial>
    -   Administrator access: SXX

Other media:

-   Steam group: <https://steamcommunity.com/groups/VCMI>
    -   Administrator access: SXX
    -   Moderator access: Dydzio
-   Sub Reddit: <https://reddit.com/r/vcmi/>
    -   Administrator access: SXX
-   ModDB entry: <http://www.moddb.com/engines/vcmi>
    -   Administrator access: SXX

### Communication channels

-   Slack team: <https://h3vcmi.slack.com/>
    -   Owner: vmarkovtsev
    -   Administrator access: SXX, Warmonger, AVS...
-   Trello team: <https://trello.com/vcmi/>
    -   Administrator access: SXX
-   Unofficial discord:
    -   Owner: dydzio
    -   Administrator access: SXX
-   Unofficial IRC channel: irc.freenode.net #vcmi

### Other services

-   Launchpad PPA: <https://launchpad.net/~vcmi>
    -   Member access: AVS
    -   Administrator access: Ivan, SXX
-   Snapcraft Dashboard: <https://dashboard.snapcraft.io/>
    -   Administrator access: SXX
-   Coverity Scan page: <https://scan.coverity.com/projects/vcmi>
    -   Administrator access: SXX, Warmonger, AVS
-   OpenHub page: <https://www.openhub.net/p/VCMI>
    -   Administrator access: Tow
-   Docker Hub organization: <https://hub.docker.com/u/vcmi/>
    -   Administrator access: SXX

Reserve accounts for other code hosting services:

-   GitLab organization: <https://gitlab.com/vcmi/>
    -   Administrator access: SXX
-   BitBucket organization: <https://bitbucket.org/vcmi/>
    -   Administrator access: SXX

## What's to improve

1.  Encourage Tow to transfer VCMI.eu to GANDI so it's can be also
    renewed without access.
2.  Use 2FA on CloudFlare and just ask everyone to get FreeOTP and then
    use shared secret.
3.  Centralized way to post news about game updates to all social media.