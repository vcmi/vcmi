# Introduction

The purpose of VCMI project is to rewrite entire HoMM3: WoG engine from
scratch, giving it new and extended possibilities. We are hoping to
support mods and new towns already made by fans, but abandoned because
of game code limitations.
VCMI is a fan-made open-source project in progress. We already allow
support for maps of any sizes, higher resolutions and extended engine
limits. However, although working, the game is not finished. There are
still many features and functionalities to add, both old and brand new.
Learn more about VCMI Project at
[Wiki](http://wiki.vcmi.eu/index.php?title=VCMI).\
Check [google
docs](http://spreadsheets.google.com/ccc?key=pRhYM0YkAF9lIpLe4raNAWA&hl=pl)
for the list of already implemented objects, spells and artifacts.

# Installation

VCMI requires Heroes of Might & Magic 3 complete + WoG 3.58f
installation and will not run properly without their files. We strongly
recommend using English version, other languages may cause unexpected
errors or bizarre font glitches.\
If you don't have Wake of Gods 3.58f yet, click
[here](http://www.maps4heroes.com/heroes3/files/allinone_358f.zip).\
For English language files, extract [this
package](http://download.vcmi.eu/dataEN.7z) to your `Data` folder.\
Starting from 0.90, VCMI can be also installed on H3 + ERA setups.

## Windows

To install VCMI, simply unzip downloaded archive to main HoMM3
directory. To launch it, click `VCMI_client` icon. Server mode is
inactive yet.\

## Linux

Visit [Wiki
article](http://wiki.vcmi.eu/index.php?title=Installation_on_Linux) for
Linux packages and installation guidelines.\

# New features

A number of enchancements had been introduced thorough new versions of
VCMI. In this section you can learn about all of them.

## High resolutions

VCMI supports resolutions higher than original 800x600.
Switching resolution may not only change visible area of map, but also alters some interface features such as [Stack Queue.](#Stack_Queue)
To change resolution or full screen mode use System Options menu when in game. Fullscreen mode can be toggled anytime using F4 hotkey.

## Stack Experience

In 0.85, new stack experience interface has been merged with regular creature window. Among old functionalities, it includes new useful info:

- Click experience icon to see detailed info about creature rank and experience needed for next level. This window works only if stack experience module is enabled (true by default).
- Stack Artifact. As yet creature artifacts are not handled, so this place is unused. You can choose enabled artifact with arrow buttons. There is also additional button below to pass currently selected artifact back to hero.
- Abilities description contain information about actual values and types of bonuses received by creature - be it default ability, stack experience, artifact or other effect. These descriptions use custom text files which have not been translated.

## Commanders

VCMI offers native support for Commanders. By default, they resemble original WoG behaviour with basic "Commanders" script enabled.

## Stack Queue

Stack queue is a feature coming straight from HoMM5, which allows you to see order of stacks on the battlefield, sorted from left to right. To toggle in on/off, press 'Q' during the battle. There is smaller and bigger version of it, the second one is available only in higher resolutions.

## Pathfinder

VCMI introduces improved pathfinder, which may find the way on adventure map using ships and subterranean gates. Simply click your destination on another island or level and the proposed path will be displayed.

## Quest log

In 0.9 new quest log was introduced. It can display info about Seer Hut or Quest Guard mission, but also handle Borderguard and Border Gate missions. When you choose a quest from the list on the left, it's description is shown. Additionally, on inner minimap you can see small icons indicating locations of quest object. Clicking these objects immediately centers adventure map on desired location.

## Attack range

In combat, some creatures, such as Dragon or Cerberi, may attack enemies
on multiple hexes. All such attacked stacks will be highlighted if the
attack cursor is hovered over correct destination tile.
Whenever battle stack is hovered, its movement range is highlighted in
darker shade. This can help when you try to avoid attacks of melee
units.

## Power rating

When hovering cursor over neutral stack on adventure map, you may notice
additional info about relative threat this stack poses to selected hero.
This feature has been introduced in Heroes of Might and Magic V and is
planned to be extended to all kinds of armed objects.
Custom text file is in use, so using localized version of data files
will not change text in your game. It is not a bug, but lack of new
translated files.

## FPS counter

VCMI 0.85 introduces new feature for testing, the FPS counter. To enable
it, edit this line in config/settings.txt file:\
[`showFPS=0;`]{style="background-color: SpringGreen"}\
Value of 1 enables simple FPS counter which may be useful to test
graphical performance on mobile platforms.

## Custom menu graphics

Since 0.9, it is possible to use any background graphics in main menu.
Open `config/mainmenu.json` file for that reason and edit this line:\
[`"background" : "background-file-name"`]{style="background-color: SpringGreen"}\
Place the background file in `Data` folder.

## Minor improvements

### Linux directory

In Linux-based sysems, files placed in $\sim$`/.vcmi` directory will
override data files with the same name.

## New controls

VCMI introduces several minor improvements and new keybinds in user
interface.

### Pregame - Scenario / Saved Game list

-   Mouse wheel - scroll through the Scenario list.

-   Home - move to the top of the list.

-   End - move to the bottom of the list.

-   NumPad keys can be used in the Save Game screen (they didn't work in
    H3).

### Adventure Map

-   CTRL + R - Quick restart of current scenario.

-   CTRL + Arrows - scrolls Adventure Map behind an open window.

-   CTRL pressed blocks Adventure Map scrolling (it allows us to leave
    the application window without losing current focus).

-   NumPad 5 - centers view on selected hero.

-   NumPad Enter functions same as normal Enter in the game (it didn't
    in H3).

### Spellbook

-   ALT + 1-10 or '-' or '=' on main pad - cast 1st to 12th visible
    spell

-   ALT + 1-10 or '-' or '+' on NumPad - cast 1st to 12th spell

### Miscellaneous

-   Numbers for components in selection window - for example Treasure
    Chest, skill choice dialog and more yet to come.

-   Type numbers in the Split Stack screen (for example 25 will split
    the stacks as such that there are 25 creatures in the second stack).

-   'Q' - Toggles the [Stack Queue](#Stack_Queue) display (so it can be
    enabled/disabled with single key press).

-   During Tactics phase, click on any of your stack to instantly
    activate it. No need to scroll trough entire army anymore.

## Cheat codes

Following cheat codes have been implemented in VCMI. Type them in
console:

-   `vcmiistari` - Gives all spells and 999 mana to currently selected
    hero

-   `vcmiainur` - Gives 5 Archangels to every empty slot of currently
    selected hero

-   `vcmiangband` - Gives 10 Black Knights into each slot

-   `vcmiarmenelos` - Build all structures in currently selected town

-   `vcminoldor` - All war machines

-   `vcminahar` - 1000000 movement points

-   `vcmiformenos` - Give resources (100 wood, ore and rare resources
    and 20000 gold)

-   `vcmieagles` - Reveals fog of war

-   `vcmiglorfindel` - Advances currently selected hero to the next
    level

-   `vcmisilmaril` - Player wins

-   `vcmimelkor` - Player loses

-   `vcmiforgeofnoldorking` - Hero gets all artifacts except spell book,
    spell scrolls and war machines

## Command line

It is possible to save a starting configuration (such as map and
options) in pregame by typing \"`sinfo` filename\". Then VCMI can be
started with option `-i –start=fname` and it will automatically start
the game.\
`–onlyAI` command line option allows to run AI-on-AI game (without GUI).
Also, typing `onlyai` in pregame triggers that mode.

# Release notes

-   In 0.89 [Commanders](#Commanders) and [Stack
    Artifacts](#Stack_Artifacts) were implemented and enabled by
    default. There are four Stack Artifacts available. Their behaviour
    has been altered for testing purpose only.

-   [Stack Experience](#Stack_Experience) is enabled by default. It's an
    optional feature, but needs complex tesing. Mechanics and tables
    from original WoG 3.58f were used for this purpose. Note that not
    all of WoG creature abilities are handled.

-   Online game, random map generator as well as some other parts of the
    game are not yet implemented. Do not worry if nothing happens when
    you click them, please report only actual bugs and game crashes.

-   It is possible to start the campaign, although heroes will not carry
    over to subsequent scenarios.

# Feedback

Our project is open and its sources are available for everyone to browse
and download. We do our best to inform community of Heroes fans with all
the details and development progress. We also look forward to your
comments, support and advice.\
A good place to start is [VCMI
Wiki](http://wiki.vcmi.eu/index.php?title=Main_Page) which contains all
necessary information for developers, testers and the people who would
like to get familiar with our project. If you want to report a bug, use
[Mantis Bugtracker](http://bugs.vcmi.eu/bug_report_advanced_page.php).\
Make sure the issue is not already mentioned on [the
list](http://bugs.vcmi.eu/view_all_bug_page.php) unless you can provide
additional details for it.\
Please do not report as bugs features not yet implemented. For proposing
new ideas and requests, visit [our
board](http://forum.vcmi.eu/index.php).\
VCMI comes with its own bug handlers: the console which prints game log
`(server_log, VCMI_Client_log, VCMI_Server_log)` and memory dump file
(`.dmp`) created on crash on Windows systems. These may be very helpful
when the nature of bug is not obvious, please attach them if necessary.\
To resolve an issue, we must be able to reproduce it on our computers.
Please put down all circumstances in which a bug occurred and what did
you do before, especially if it happens rarely or is not clearly
visible. The better report, the better chance to track the bug quickly.

# FAQ

### What does "VCMI" stand for?

VCMI is an acronym of the [Quenya](https://en.wikipedia.org/wiki/Quenya)
phrase "Vinyar Callor Meletya Ingole", meaning "New Heroes of Might and
Magic". ([Source](https://forum.vcmi.eu/t/what-vcmi-stands-for/297/4))

## How to turn off creature queue panel in battles?

Hotkey to switch this panel is "Q"

### Is it possible to add town X to vcmi?

This depends on town authors or anyone else willing to port it to vcmi.
Aim of VCMI is to provide *support* for such features.

## When will the final version be released?

When it is finished, which is another year at least. Exact date is
impossible to predict.\
Development tempo depends mostly on free time of active programmers and
community members, there is no exact shedule. You may expect new version
every three months. Of course, joining the project will speed things up.

## Are you going to add / change X?

VCMI recreates basic H3:TSoD + WoG engine and does not add new content
or modify original mechanics by default. Only engine and interface
improvements are likely to be supported now. The list of possible
features is placed on [Wiki TODO
list](http://wiki.vcmi.eu/index.php?title=TODO_list). If you want
something specific to be done, please present detailed project on [our
board](http://forum.vcmi.eu/index.php). Of course you are free to
contribute with anything you can do.

## Will it be possible to do Y?

Removing engine restrictions and allowing flexible modding of game is
the main aim of the project.\
As yet modification of game is not supported.

## The game is not working, it crashes and I get strange console messages.

Report your bug. Details are described [here](#Feedback). The sooner you
tell the team about the problem, the sooner it will be resolved. Many
problems come just from improper installation or system settings.

## What is the current status of the project?

Check [Wiki](http://wiki.vcmi.eu/index.php?title=VCMI), [release
notes](http://forum.vcmi.eu/viewforum.php?f=1) or
[changelog](https://github.com/vcmi/vcmi/blob/develop/ChangeLog). The
best place to watch current changes as they are committed to the develop
branch is the [Github commits
page](https://github.com/vcmi/vcmi/commits/develop). The game is quite
playable by now, although many important features are missing.

## I have a great idea!

Share it on [VCMI forum](http://forum.vcmi.eu/index.php) so all team
members can see it and share their thoughts. Remember, brainstorming is
good for your health.

## Are you going to support Horn of The Abyss / Wog 3.59 / Grove Town etc.?

Yes, of course. VCMI is designed as a base for any further mods and uses
own executables, so the compatibility is not an issue. The team is not
going to compete, but to cooperate with the community of creative
modders.

## I don't like Wake of Gods at all. Can I disable it?

Most WoG features already implemented are just for testing purposes and
can be disabled in [mod settings](#Mods).

## Can I help VCMI Project in any way?

If you are C++ programmer, graphican, tester or just have tons of ideas,
do not hesistate - your help is needed. The game is huge and many
different ares of activity are still waiting for someone like you. See
[Wiki TODO list](http://wiki.vcmi.eu/index.php?title=TODO_list) for more
info.

## I would like to join development team.

You are always welcome. Contact the core team via [our
board](http://forum.vcmi.eu/index.php). In the meantime, read 'building
VCMI' guide at [Wiki](http://wiki.vcmi.eu/index.php?title=Main_Page).\
The usual way to join the team is to post your patch for review on our
board. If the patch is positively rated by core team members, you will
be given access to SVN repository.

# Credits