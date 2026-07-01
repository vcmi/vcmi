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

- returns `MapObject`
