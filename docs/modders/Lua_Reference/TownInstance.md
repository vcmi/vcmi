# TownInstance

A town on the adventure map. Provides its owner and what has been built in it.

### getOwner

Returns the player color that owns this town, or the neutral player when it is unowned.

- returns `integer`

### getBuildings

Returns the buildings that have been built in this town, upgrades of other buildings among them.

- returns [`Building[]`](Building.md) — Every building standing in this town.

### getVisitingHero

Returns the hero that stands in the town entrance, or nil when there is none.

- returns [`HeroInstance`](HeroInstance.md) — Hero visiting the town, or nil.

### getGarrisonHero

Returns the hero that guards the town as part of its garrison, or nil when there is none.

- returns [`HeroInstance`](HeroInstance.md) — Hero in the town garrison, or nil.

### getFaction

Returns the faction (Castle, Rampart, Tower...) of this town.

- returns [`Faction`](Faction.md) — Faction of the town, comparable with the result of Services:getFactionByName.
