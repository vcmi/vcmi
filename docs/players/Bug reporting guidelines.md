First of all, thanks for your support! If you report a bug we can fix
it. But keep in mind that reporting your bugs appropriately makes our
(developers') lifes easier. Here are a few guidelines that will help you
write good bug reports.

# Mantis bugtracker

The main place for managing and reporting bugs is [our
bugtracker](http://bugs.vcmi.eu/). When you are not logged in, you can
only browse already reported bugs. To be able to report bugs you need to
make an account there.

## What should be reported

Certainly the most important bugs we would like to know about are
crashes and game hangs. Game should not crash nor hang under any
conditions. But bugs are not restricted to those extreme cases.
Graphical glitches, significant differences in game mechanics from WoG
(in this case remember that not everything is implemented yet, we
usually don't call missing features as bugs; see [TODO
list](TODO_list "wikilink") for details about what is still to be done)
and serious performance drops should be reported too.

## What to focus on while testing

There are no specific guidelines on this. Every part of the game needs
some attention while testing. Usually newly added features should be
tested more. Sometimes bugs occur only when loading from savegame, so
you shouldn't always begin a new game.

## General guidelines

First of all, if you encounter a crash, don't re-run VCMI immediately to
see if you can reproduce it. Firstly take a screenshot or copy console
output (those mostly green letters on black background). Then back up
following files (if you won't be able to reproduce the issue you should
upload them with issue report):

-   VCMI_Client_log.txt
-   VCMI_Server_log.txt
-   VCMI_Client.exe_crashinfo.dmp (if present)
-   VCMI_Server.exe_crashinfo.dmp (if present)

By default, log files are written to:

-   Windows: %USERPROFILE%\Documents\My Games\vcmi\\
-   UNIX: ~/.cache/vcmi/

Now you should try to reproduce encountered issue. It's best when you
write how to reproduce the issue by starting a new game and taking some
steps (e.g. start Arrogance map as red player and attack monster Y with
hero X). If you have troubles with reproducing it this way but you can
do it from a savegame - that's good too. Finally, when you are not able
to reproduce the issue at all, just upload the files mentioned above. To
sum up, this is a list of what's the most desired for a developer:

1.  (most desired) a map with list of steps needed to reproduce the bug
2.  savegame with list of steps to reproduce the bug
3.  (least desired) VCMI_Client_log.txt, VCMI_Server_log.txt, console
    log and crashdump (you should use this option only when bug is not
    reproducible but then remember to back logs up before trying to
    reproduce it).

## How to set the log level to debug

If you want to set the log level to debug or trace to let developers
know what went wrong or to write a bug report, then you should first
open your `settings.json` with a text editor. The file is located at:

-   Windows: VCMI installation location\config\schemas\\ (I.e.
    C:\Program Files (x86)\VCMI (branch develop)\config\schemas\\
-   UNIX: ~/.config/vcmi/

Add the "logging" : { ... } part to the file, that it may look like
this:

``` javascript
{
	"logging" : {
		"loggers" : [
			{
				"domain" : "global",
				"level" : "debug"
			}
		]
	}
}
```

You can substitute the value debug with trace to log traces as well.