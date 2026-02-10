# Lua Scripting System

## Configuration

```json
{
	//general purpose script, Lua, runs on server
 	"myScript":
	{
		"source":"path/to/script/with/ext",
		"implements":"ANYTHING"
	},


 	//custom battle spell effect, Lua only, runs on both client and server
 	//script ID will be used as effect 'type' (with mod prefix)
 	"mySpellEffect":
	{
		"source":"path/to/script/with/ext",
		"implements":"BATTLE_EFFECT"
	},

	//TODO: object|building type
 	//custom map object or building, Lua only, runs on both client and server
 	//script ID will be used as 'handler' (with mod prefix)
 	"myObjectType":
	{
		"source":"path/to/script/with/ext",
		"implements":"MAP_OBJECT"
	},
	//TODO: server query
	//TODO: client query

}
```

## Lua

### API Reference

TODO **In near future Lua API may change drastically several times. Information here may be outdated**

#### Globals

- DATA - persistent table
- GAME - IGameInfoCallback API
- BATTLE - IBattleInfoCallback API
- EVENT_BUS - opaque handle, for use with events API
- SERVICES - root "raw" access to all static game objects
- - SERVICES:artifacts()
- - SERVICES:creatures()
- - SERVICES:factions()
- - SERVICES:heroClasses()
- - SERVICES:heroTypes()
- - SERVICES:spells()
- - SERVICES:skills()
- require(URI)
- -works similar to usual Lua require
- -require("ClassName") - loads additional API and returns it as table (for C++ classes)
- -require("core:relative.path.to.module") - loads module from "SCRIPTS/LIB"
- -TODO require("modName:relative.path.to.module") - loads module from dependent mod
- -TODO require(":relative.path.to.module") - loads module from same mod
- logError(text) - backup error log function

#### Low level events API

```lua

-- Each event type must be loaded first
local PlayerGotTurn = require("events.PlayerGotTurn")

-- in this example subscription handles made global, do so if there is no better place
-- !!! do not store them in local variables
sub1 = 	PlayerGotTurn.subscribeAfter(EVENT_BUS, function(event)
		--do smth
	end)

sub2 = 	PlayerGotTurn.subscribeBefore(EVENT_BUS, function(event)
		--do smth
	end)
```

#### Lua standard library

VCMI uses LuaJIT, which is Lua 5.1 API, see [upstream documentation](https://www.lua.org/manual/5.1/manual.html)

Following libraries are supported

- base
- table
- string
- math
- bit
