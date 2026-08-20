# TownInstance

A town on the adventure map. Provides its owner and what has been built in it.

### getOwner

Returns the player color that owns this town, or the neutral player when it is unowned.

- returns `integer`

### getBuildings

Returns the buildings that have been built in this town, upgrades of other buildings among them.

- returns [`Building[]`](Building.md) — Every building standing in this town.
