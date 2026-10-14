# Building

A building of a town, as `TownInstance:getBuildings` reports it.

### getJsonKey

Returns the json key of this building, such as `core:fort`.

- returns `string` — Identifier of this building, scoped by the mod providing it.

### getBuildingType

Returns which of the buildings known to the game this one is. Unlike the json key this is the same in every town, so it is what to test against when a rule speaks of a fort or a town hall rather than of one particular mod's version of it.

- returns `string?` — "fort", "villageHall", ...; nil for a building the game has no name of its own for.

### isUpgrade

Whether this building is an upgrade of another, as a citadel is of a fort.

- returns `boolean` — True when this building improves another one instead of standing on its own.
