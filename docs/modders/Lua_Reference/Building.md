# Building

A building of a town, as `TownInstance:getBuildings` reports it.

### getJsonKey

Returns the json key of this building, such as `core:fort`.

- returns `string` — Identifier of this building, scoped by the mod providing it.

### getBuildingType

Returns the predefined building type shared across towns.

- returns `string?` — 'fort', 'villageHall', ...; nil for buildings without a predefined type.

### isUpgrade

Whether this building is an upgrade of another, as a citadel is of a fort.

- returns `boolean` — True when this building improves another one instead of standing on its own.
