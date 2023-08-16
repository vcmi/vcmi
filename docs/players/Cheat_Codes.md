See also: [Client Commands](Client_Commands.md)

Similar to H3, VCMI provides cheat codes to make testing game more convenient.

To use cheat code, press `Tab` key or click/tap on status bar to open game chat and enter code. Most cheat codes have several alternative names, including name of this cheat code in H3:SoD

## List of cheat codes

### Spells

`nwcthereisnospoon` or `vcmiistari` or `vcmispells` - give a spell book, all spells and 999 mana to currently selected hero

### Army

`nwctrinity` or `vcmiainur` or `vcmiarchangel` - give 5 Archangels in every empty slot (to currently selected hero)  
`nwcagents` or `vcmiangband` or `vcmiblackknight` - give 10 black knight in every empty slot  
`vcmiglaurung` or `vcmicrystal` - give 5000 crystal dragons in every empty slot  
`vcmiazure` - give 5000 azure dragons in every empty slot  
`vcmifaerie` - give 5000 faerie dragons in every empty slot  

Alternative usage: `vcmiarmy <creatureID> <amount>`
Gives specific creature in every slot, with optional amount. Examples:
`vcmiarmy imp` - give 5, 50, 500... 500k imps in every free slot
`vcmiarmy grandElf 100` - gives 100 grand elves in every free slot

### Town buildings

`nwczion` or `vcmiarmenelos` or `vcmibuild` - build all buildings in currently selected town

### Artifacts

`nwclotsofguns` or `vcminoldor` or `vcmimachines` - give ballista, ammo cart and first aid tent  
`vcmiforgeofnoldorking` or `vcmiartifacts` - give all artifacts, except spell book, spell scrolls and war machines. Artifacts added via mods included  

### Movement points

`nwcnebuchadnezzar` or `vcminahar` or `vcmimove` - give 1000000 movement points and free ship boarding for 1 day  
Alternative usage: `vcmimove <amount>` - gives specified amount of movement points

### Resources

`nwctheconstruct` or `vcmiformenos` or `vcmiresources` - give resources (100000 gold, 100 of wood, ore and rare resources)  
Alternative usage: `vcmiresources <amount>` - gives specified amount of all resources and x1000 of gold

### Fog of War

`nwcwhatisthematrix` or `vcmieagles` or `vcmimap` - reveal Fog of War  
`nwcignoranceisbliss` or `vcmiungoliant` or `vcmihidemap` - conceal Fog of War  

### Experience

`nwcneo` or `vcmiglorfindel` or `vcmilevel` - advances currently selected hero to the next level
Alternative usage: `vcmilevel <amount>` - advances hero by specified number of levels

- `vcmiolorin` or `vcmiexp` - gives selected hero 10000 experience
Alternative usage: `vcmiexp <amount>` - gives selected hero specified amount of experience

### Finishing the game

`nwcredpill` or `vcmisilmaril` or `vcmiwin` - player wins  
`nwcbluepill` or `vcmimelkor` or `vcmilose` - player loses  

## Using cheat codes on other players
By default, all cheat codes apply to current player. Alternatively, it is possible to specify player that you want to target:

- Specific players: `red`/`blue`/`green`...
- Only AI players: `ai` 
- All players: `all`

### Examples

`vcmieagles blue` - reveal FoW only for blue player  
`vcmieagles ai` - reveal FoW only for AI players  
`vcmieagles all` - reveal FoW for all players on map  
`vcminahar ai` - give 1000000 movement points to each hero of every AI player  

## Multiplayer chat commands
Note: These commands are not a cheats, and can be used in multiplayer by host player to control the session

- `game exit/quit/end` - finish the game  
- `game save <filename>` - save the game into the specified file  
- `game kick red/blue/tan/green/orange/purple/teal/pink` - kick player of specified color from the game  
- `game kick 0/1/2/3/4/5/6/7/8` - kick player of specified ID from the game (_zero indexed!_) (`0: red, 1: blue, tan: 2, green: 3, orange: 4, purple: 5, teal: 6, pink: 7`)  