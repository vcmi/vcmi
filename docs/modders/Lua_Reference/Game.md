# Game

Adventure-map query interface. Provides world-level lookups: current date, players, towns, heroes, and map objects accessible to the calling script's owner.

### getHero

Returns the hero by its object identifier, or nil if not found.

- param `objectID`: `integer` — Map object identifier of the hero to fetch.

- returns [`HeroInstance`](HeroInstance.md)

### getObj

Returns the map object by its identifier, or nil if not found.

- param `objectID`: `integer` — Map object identifier of the object to fetch.
- param `verbose`: `boolean` — Pass true to log a warning when the object isn't found.

- returns [`MapObject`](MapObject.md)

### getResource

Returns the amount of the given resource owned by the player.

- param `player`: `integer` — Player whose treasury is queried.
- param `resource`: [`ResourceType`](ResourceType.md) — Resource to read, as returned by Services:getResourceByName.

- returns `integer` — Amount of that resource the player currently owns.

### getCalendar

Returns the calendar object for the current in-game date.

- returns [`Calendar`](Calendar.md) — Calendar for the current in-game date.

### getDifficulty

Returns the current game difficulty level.

- returns [`Difficulty`](Difficulty.md) — Current game difficulty; compare against ENUM.Difficulty (pawn..king).

### getMapVariable

Reads back a named value saved earlier with AdventureServer:setMapVariable. Values survive saving and loading, so this is how a script remembers state between visits.

- param `name`: `string` — Name of the variable to read, as stored with AdventureServer:setMapVariable.

- returns `any` — The stored value, or nil when nothing was ever stored under that name.

### hasMapVariable

Checks whether a named map variable has ever been set, which lets a script tell "unset" apart from a stored value of 0 or false.

- param `name`: `string` — Name of the variable to check.

- returns `boolean` — True when a value has been stored under that name.

### playerIsHuman

Tells whether the given player is controlled by a human.

- param `player`: `integer` — Player color index to check.

- returns `boolean` — True when that player is controlled by a human, false for an AI or an unused color.

### getPlayerStatus

Returns whether the player is still playing, has won, or has been defeated.

- param `player`: `integer` — Player whose status is queried.

- returns [`PlayerStatus`](PlayerStatus.md) — Current status; compare against ENUM.PlayerStatus.

### getPlayerFaction

Returns the town faction a player started the map with.

- param `player`: `integer` — Player whose starting faction is queried.

- returns [`Faction`](Faction.md) — The town faction the player began the map with, or nil when the player has none.

### wasQuestProposed

Tells whether a player has already seen the object's current quest proposed, so a script can show the progression text instead of the proposal text on a repeat visit.

- param `target`: [`MapObject`](MapObject.md) — The quest source (seer hut / quest guard) to check.
- param `player`: `integer` — Player to check.

- returns `boolean` — True once the player has already been offered the object's active quest.

### getHeroByType

Finds the hero of the given type placed on the map.

- param `heroType`: [`HeroType`](HeroType.md) — Hero type to look for, as returned by Services:getHeroTypeByName.

- returns [`HeroInstance`](HeroInstance.md) — The hero of that type currently on the map, or nil when no such hero exists.

### playerDestroyedObject

Tells whether the given player destroyed the given map object.

- param `player`: `integer` — Player to check.
- param `target`: [`MapObject`](MapObject.md) — Map object to check, e.g. a wandering monster or a hero.

- returns `boolean` — True when that player destroyed the object.

### getObjectByName

Looks up a map object by its instance name.

- param `objectName`: `string` — Instance name of the object, as resolved through the questObjects table.

- returns [`MapObject`](MapObject.md) — The map object with that instance name, or nil when the name is empty or unknown.
