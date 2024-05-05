First of all, thanks for your support! If you report a bug we can fix it. But keep in mind that reporting your bugs appropriately makes our (developers') lifes easier. Here are a few guidelines that will help you write good bug reports.

# Github bugtracker

The main place for managing and reporting bugs is [our bugtracker](https://github.com/vcmi/vcmi/issues). When you are not logged in, you can only browse already reported bugs. To be able to report bugs you need to make an account there.

## What should be reported

Certainly the most important bugs we would like to know about are crashes and game hangs. Game should not crash nor hang under any conditions. But bugs are not restricted to those extreme cases. Graphical glitches, significant differences in game mechanics and serious performance drops should be reported too.

## What to focus on while testing

There are no specific guidelines on this. Every part of the game needs some attention while testing. Usually newly added features should be tested more. Sometimes bugs occur only when loading from savegame, so you shouldn't always begin a new game.

## General guidelines

First of all, if you encounter a crash, don't re-run VCMI immediately to see if you can reproduce it. Firstly take a screenshot or copy console output (those mostly green letters on black background). Then back up following files (if you won't be able to reproduce the issue you should upload them with issue report):

- VCMI_Client_log.txt
- VCMI_Server_log.txt

By default, log files are written to:

-   Windows: Documents\My Games\vcmi\\
-   Linux: ~/.cache/vcmi/
-   Android: Android/data/is.xyz.vcmi/files/vcmi-data/cache/

Now you should try to reproduce encountered issue. It's best when you write how to reproduce the issue by starting a new game and taking some steps (e.g. start Arrogance map as red player and attack monster Y with hero X). If you have troubles with reproducing it this way but you can do it from a savegame - that's good too. Finally, when you are not able to reproduce the issue at all, just upload the files mentioned above. To sum up, this is a list of what's the most desired for a developer:

1. (most desired) a map with list of steps needed to reproduce the bug
2. savegame with list of steps to reproduce the bug
3. (least desired) VCMI_Client_log.txt and VCMI_Server_log.txt (but then remember to back logs up before trying to reproduce it).
