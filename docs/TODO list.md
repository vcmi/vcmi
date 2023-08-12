The list of all essential, optional and requested features alongside
with the team members interested in them.

# Heroes III

[Missing features at
Trello](https://trello.com/b/68e5rAAl/vcmi-missing-features-only)

### Most important bugfixes

-   daily build contains broken vcmi essential package (ticket is for
    linux but same problem on Windows also)
    <https://bugs.vcmi.eu/view.php?id=2986> - questionable
-   desync in multiplayer cross-platform
    <https://bugs.vcmi.eu/view.php?id=2583>

### Establish release process

In January 2021 there was a proposition on Slack to make 3 branches:
develop, beta and stable. develop is where people make their changes
when they are ready, people can test it but its not for normal players
beta could be for players who agree to be testers. stable could be for
players who want stable version. Hovewer there were no decision made

### HD mod/quality of life

contact MikeLodz if you have questions about this section

These are necessary, as although they werent present back in 1999, they
are standard today.

-   quick transfer of artifacts and armies between heroes, started here
    <https://github.com/vcmi/vcmi/pull/636>
-   adventure map: movements points preview and cost
-   move artifacts between visiting town and garrison heroes
-   hero list preview on the "start map" screen.

etc see here for full list of enhancements:
<https://sites.google.com/site/heroes3hd/eng/description/extended-ui>

-   some walking creatures need to move faster during battles, as its a
    complete waste of time (we need a so called "turbo" mode)

<!-- -->

-   this one not sure if its a bug or missing hd mod feature: it should
    be possible to preview hero screen when choosing a skill on levelup.

### Hota rebalances?

contact MikeLodz about this section

-   what is our stance on Hota rebalances ? we should discuss them at
    least and decide which to adopt, and which to ignore \*

### Random map generator

Dydzio said about it on Slack:

`finish `[`https://github.com/dydzio0614/vcmi/tree/rmgtemplates`](https://github.com/dydzio0614/vcmi/tree/rmgtemplates)` (random map generator template pick,`

better selection lists for random map generator template - two versions
(two bitmaps provided) also one more thing - for random map template
pick my vision was to create more generic select list component and make
town portal select component derive from it

### [Adventure AI](Adventure_AI "wikilink")

-   More functionality
    -   Adventure map spells support
    -   Survival instinct - AI defending towns, escaping etc.
    -   Handling of all adventure map objects
-   Evaluating game objects
    -   priority for visited objects over others sharing similiar
        function
    -   Support for new objects possible to add via mods - use abstract
        interface to determine rewards etc.
-   Advanced strategy
    -   Battle preparation (constructing suitable army & strategy)
    -   Expert system for Bonuses
        ([Warmonger](http://forum.vcmi.eu/profile.php?mode=viewprofile&u=130))

### Main menu

-   Hall of Fame

# Modding

-   Possibility for creatures to cast arbitrary spells, as in H4/H5

### Scripting system

-   Language support
-   Synchronising scripts in multiplayer

### Mod system

-   Adding new content
    -   Adventure map objects
    -   Battlefields

[Forum
thread](http://forum.vcmi.eu/viewtopic.php?t=471&postdays=0&postorder=asc&start=0)

### Wog features

-   Mithril
-   ERM handling (Tow, Tow Dragon)

# New features

-   Support for other languages files
    ([Ivan](http://forum.vcmi.eu/profile.php?mode=viewprofile&u=336))
-   Support for multiple map levels
-   [New map editor](http://forum.vcmi.eu/viewtopic.php?t=1139)
-   Installer

### Online game

-   Simultaneous turns
-   Spectator mode
-   Dedicated server mode
-   [Replays](http://forum.vcmi.eu/viewtopic.php?t=264)

### Other platforms

-   Linux (<span style="color:green">**Supported**</span>, [forum
    thread](http://forum.vcmi.eu/viewtopic.php?t=112))
-   Mac OS (<span style="color:green">**Supported**</span>, [forum
    thread](http://forum.vcmi.eu/viewtopic.php?t=439))
-   Android (<span style="color:green">**Supported**</span>, [forum
    thread](http://forum.vcmi.eu/viewtopic.php?t=850))
-   iOS (<span style="color:green">**Supported**</span>, [forum
    thread](https://forum.vcmi.eu/t/ios-port/820))
-   Haiku (<span style="color:yellow">**Status unknown**</span>, [forum
    thread](http://forum.vcmi.eu/viewtopic.php?t=310))
-   Maemo (<span style="color:yellow">**Status unknown**</span>, [forum
    thread](http://forum.vcmi.eu/viewtopic.php?t=328))
-   FreeBSD (<span style="color:yellow">**Status unknown**</span>)

# New graphics

### New menus project and graphics

-   New stack experience menu ([forum
    thread](http://forum.vcmi.eu/viewtopic.php?t=376&start=0))
-   New stack artifact dialog

### Improved support for high resolutions

-   Auto-adjust resolution
-   Moddable menus

### New players

-   More player color graphics
-   New menus

### 32-bit graphics

### 800 x 480 resolution

Experimental version posted on our forum is available
[here](http://forum.vcmi.eu/viewtopic.php?t=273).