See also: [Cheat Codes](Cheat_Codes.md)

Client commands are set of predefined commands that are supported by VCMI, but unlike cheats they perform utility actions that do not alter state of the gameplay. As of release 1.2 client commands can work by typing them in-game like cheats, preceded by symbol / (for example `/controlai blue`)

Alternative way, the only one working for older releases is typing them in console:
Console is separated from game window on desktop versions of VCMI Client.
Windows builds of VCMI run separate console window by default, on other platforms you can access it by running VCMI Client from command line.  

Below a list of supported commands, with their arguments wrapped in `<>`

#### Game Commands
`die, fool` - quits game  
`save <filename>` - saves game in given file (at the moment doesn't work)  
`mp` - on adventure map with a hero selected, shows heroes current movement points, max movement points on land and on water  
`bonuses` - shows bonuses of currently selected adventure map object

#### Extract commands
`convert txt` - save game texts into json files  
`get config` - save game objects data into json files  
`get scripts` - dumps lua script stuff into files (currently inactive due to scripting disabled for default builds)    
`get txt` - save game texts into .txt files matching original heroes 3 files  
`def2bmp <.def file name>` - extract .def animation as BMP files  
`extract <relative file path>` - export file into directory used by other extraction commands  

#### AI commands
`setBattleAI <ai name>` - change battle AI used by neutral creatures to the one specified, persists through game quit  
`gosolo` - AI takes over until the end of turn (unlike original H3 currently causes AI to take over until typed again)  
`controlai <[red][blue][tan][green][orange][purple][teal][pink]>` - gives you control over specified AI player. If none is specified gives you control over all AI players  
`autoskip` - Toggles autoskip mode on and off. In this mode, player turns are automatically skipped and only AI moves. However, GUI is still present and allows to observe AI moves. After this option is activated, you need to end first turn manually. Press `[Shift]` before your turn starts to not skip it  

#### Settings
`set <command> <on/off>` - sets special temporary settings that reset on game quit. Below some of the most notable commands:  
-`autoskip` - identical to `autoskip` option  
-`onlyAI` - run without human player, all players will be _default AI_  
-`headless` - run without GUI, implies `onlyAI` is set  
-`showGrid` - display a square grid overlay on top of adventure map  
-`showBlocked` - show blocked tiles on map  
-`showVisitable` - show visitable tiles on map  
-`hideSystemMessages` - suppress server messages in chat  

#### Developer Commands
`crash` - force a game crash. It is sometimes useful to generate memory dump file in certain situations, for example game freeze  
`gui` - displays tree view of currently present VCMI common GUI elements  
`activate <0/1/2>` - activate game windows (no current use, apparently broken long ago)  
`redraw` - force full graphical redraw  
`screen` - show value of screenBuf variable, which prints "screen" when adventure map has current focus, "screen2" otherwise, and dumps values of both screen surfaces to .bmp files  
`unlock pim` - unlocks specific mutex known in VCMI code as "pim"  
`not dialog` - set the state indicating if dialog box is active to "no"  
`tell hs <hero ID> <artifact slot ID>` - write what artifact is present on artifact slot with specified ID for hero with specified ID. (must be called during gameplay)  
