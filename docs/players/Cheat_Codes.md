# Cheat Codes

## Codes

Similar to H3, VCMI provides cheat codes to make testing game more convenient.

To use cheat code, press `Tab` key or click/tap on status bar to open game chat and enter code. Most cheat codes have several alternative names, including name of this cheat code in H3:SoD, H3:AB and H3:RoE

### Spells

- `nwcthereisnospoon`, `nwcmidichlorians`, `nwctim`, `vcmiistari` or `vcmispells` - give a spell book, all spells and 999 mana to currently selected hero. Also allows casting spell up to 100 times per combat round

### Secondary Skills

- `vcmiskill <skillID> <mastery>` - give a secondary skill to currently selected hero

Examples:

- `vcmiskill learning` - give expert level learning skill
- `vcmiskill leadership 2` - give advanced level leadership skill
- `vcmiskill wisdom 0` - remove wisdom skill
- `vcmiskill every` - give all skills on expert level
- `vcmiskill every 0` - remove all skills

### Army

- `nwctrinity`, `nwcpadme`, `nwcavertingoureyes`, `vcmiainur` or `vcmiarchangel` - give 5 Archangels in every empty slot (to currently selected hero)  
- `nwcagents`, `nwcdarthmaul`, `nwcfleshwound` or `vcmiangband` or `vcmiblackknight` - give 10 black knight in every empty slot  
- `vcmiglaurung` or `vcmicrystal` - give 5000 crystal dragons in every empty slot  
- `vcmiazure` - give 5000 azure dragons in every empty slot  
- `vcmifaerie` - give 5000 faerie dragons in every empty slot  

- Alternative usage: `vcmiarmy <creatureID> <amount>`

Gives specific creature in every slot, with optional amount. Examples:

- `vcmiarmy imp` - give 5, 50, 500... 500k imps in every free slot
- `vcmiarmy grandElf 100` - gives 100 grand elves in every free slot

### Town buildings

- `nwczion`, `nwccoruscant`, `nwconlyamodel`, `vcmiarmenelos` or `vcmibuild` - build all buildings in currently selected town

### Artifacts

- `nwclotsofguns`, `nwcr2d2`, `nwcantioch`, `vcminoldor` or `vcmimachines` - give ballista, ammo cart and first aid tent  
- `vcmiforgeofnoldorking` or `vcmiartifacts` - give all artifacts, except spell book, spell scrolls and war machines. Artifacts added via mods included  
- `vcmiscrolls` - give spell scrolls for every possible spells

### Movement

- `nwcnebuchadnezzar`, `nwcpodracer`, `nwccoconuts`, `vcminahar` or `vcmimove` - give unlimited (or specified amount of) movement points and free ship boarding
- Alternative usage: `vcmimove <amount>` - gives specified amount of movement points
- `vcmiteleport` - teleports hero to desired coordinate on map, usage: `vcmiteleport <x> <y> <z(optional)>`

### Resources

- `nwctheconstruct`, `nwcwatto`, `nwcshrubbery`, `vcmiformenos` or `vcmiresources` - give resources (100000 gold, 100 of wood, ore and rare resources)  
- Alternative usage: `vcmiresources <amount>` - gives specified amount of all resources and x1000 of gold

### Fog of War

- `nwcwhatisthematrix`, `nwcrevealourselves`, `nwcgeneraldirection`, `vcmieagles` or `vcmimap` - reveal Fog of War  
- `nwcignoranceisbliss`,  `vcmiungoliant` or `vcmihidemap` - conceal Fog of War  

### Experience

- `nwcneo`, `nwcquigon`, `nwcigotbetter`, `vcmiglorfindel` or `vcmilevel` - advances currently selected hero to the next level
- Alternative usage: `vcmilevel <amount>` - advances hero by specified number of levels

- `vcmiolorin` or `vcmiexp` - gives selected hero 10000 experience
- Alternative usage: `vcmiexp <amount>` - gives selected hero specified amount of experience

### Luck and morale

- `nwcfollowthewhiterabbit`, `nwccastleanthrax` or `vcmiluck` - the currently selected hero permanently gains maximum luck
- `nwcmorpheus`, `nwcmuchrejoicing` or `vcmimorale` - the currently selected hero permanently gains maximum morale

### Puzzle map

- `nwcoracle`, `nwcprophecy`, `nwcalreadygotone` or `vcmiobelisk` - reveals the puzzle map

### Finishing the game

- `nwcredpill`, `nwctrojanrabbit`, `vcmisilmaril` or `vcmiwin` - player wins
- `nwcbluepill`, `nwcsirrobin`, `vcmimelkor` or `vcmilose` - player loses

### Misc

- `nwctheone` or `vcmigod` - reveals the whole map, gives 5 archangels in each empty slot, unlimited movement points and permanent flight
- `nwcphisherprice` or `vcmicolor` - change game color palette to Heroes II like until game restart
- `vcmigray` - change game color palette to grayscale until game restart

## Using cheat codes on other players

By default, all cheat codes apply to current player. Alternatively, it is possible to specify player that you want to target:

- Specific players: `red`/`blue`/`green`...
- Only AI players: `ai`
- All players: `all`

### Examples

- `vcmieagles blue` - reveal FoW only for blue player  
- `vcmieagles ai` - reveal FoW only for AI players  
- `vcmieagles all` - reveal FoW for all players on map  
- `vcminahar ai` - give 1000000 movement points to each hero of every AI player  

## Multiplayer chat commands

Following commands can be used in multiplayer only by host player to control the session:

- `!exit` - finish the game  
- `!save <filename>` - save the game into the specified file  
- `!kick red/blue/tan/green/orange/purple/teal/pink` - kick player of specified color from the game  
- `!kick 0/1/2/3/4/5/6/7/8` - kick player of specified ID from the game (*zero indexed!*) (`0: red, 1: blue, tan: 2, green: 3, orange: 4, purple: 5, teal: 6, pink: 7`)  

Following commands can be used by any player in multiplayer:

- `!help` - displays in-game list of available commands
- `!cheaters` - lists players that have entered cheat at any point of the game
- `!vote` - initiates voting to change one of the possible options:
- `!vote simturns allow X` - allow simultaneous turns for specified number of days, or until contact
- `!vote simturns force X` - force simultaneous turns for specified number of days, blocking player contacts
- `!vote simturns abort` - abort simultaneous turns once this turn ends
- `!vote timer prolong X` - prolong base timer for all players by specified number of seconds

## Client Commands

Client commands are set of predefined commands that are supported by VCMI, but unlike cheats they perform utility actions that do not alter state of the gameplay. As of release 1.2 client commands can work by typing them in-game like cheats, preceded by symbol / (for example `/controlai blue`)

Alternative way, the only one working for older releases is typing them in console:
Console is separated from game window on desktop versions of VCMI Client.
Windows builds of VCMI run separate console window by default, on other platforms you can access it by running VCMI Client from command line.  

Below a list of supported commands, with their arguments wrapped in `<>`

#### Game Commands

- `die, fool` - quits game  
- `save <filename>` - saves game in given file (at the moment doesn't work)  
- `mp` - on adventure map with a hero selected, shows heroes current movement points, max movement points on land and on water  
- `bonuses` - shows bonuses of currently selected adventure map object

#### Extract commands

- `translate` - save game texts into json files
- `translate missing` - save untranslated game texts into json files
- `translate maps` - save map and campaign texts into json files
- `get config` - save game objects data into json files
- `get scripts` - dumps lua script stuff into files (currently inactive due to scripting disabled for default builds)
- `get txt` - save game texts into .txt files matching original heroes 3 files
- `def2bmp <.def file name>` - extract .def animation as BMP files
- `extract <relative file path>` - export file into directory used by other extraction commands
- `generate assets` - generate all assets at once

#### AI commands

- `setBattleAI <ai name>` - change battle AI used by neutral creatures to the one specified, persists through game quit  
- `gosolo` - AI takes over until the end of turn (unlike original H3 currently causes AI to take over until typed again)  
- `controlai <[red][blue][tan][green][orange][purple][teal][pink]>` - gives you control over specified AI player. If none is specified gives you control over all AI players  
- `autoskip` - Toggles autoskip mode on and off. In this mode, player turns are automatically skipped and only AI moves. However, GUI is still present and allows to observe AI moves. After this option is activated, you need to end first turn manually. Press `[Shift]` before your turn starts to not skip it  

#### Settings

- `set <command> <on/off>` - sets special temporary settings that reset on game quit. Below some of the most notable commands:
- `autoskip` - identical to `autoskip` option
- `onlyAI` - run without human player, all players will be *default AI*
- `headless` - run without GUI, implies `onlyAI` is set
- `showGrid` - display a square grid overlay on top of adventure map
- `showBlocked` - show blocked tiles on map
- `showVisitable` - show visitable tiles on map
- `showInvisible` - show invisible tiles (events, grail) on map
- `hideSystemMessages` - suppress server messages in chat
- `antilag` - toggles network lag compensation in multiplayer on or off

`showBlocked`, `showVisitable` and `showInvisible` only works if cheats are enabled.

#### Developer Commands

- `crash` - force a game crash. It is sometimes useful to generate memory dump file in certain situations, for example game freeze  
- `gui` - displays tree view of currently present VCMI common GUI elements  
- `activate <0/1/2>` - activate game windows (no current use, apparently broken long ago)  
- `redraw` - force full graphical redraw  
- `screen` - show value of screenBuf variable, which prints "screen" when adventure map has current focus, "screen2" otherwise, and dumps values of both screen surfaces to .bmp files  
- `tell hs <hero ID> <artifact slot ID>` - write what artifact is present on artifact slot with specified ID for hero with specified ID. (must be called during gameplay)  
