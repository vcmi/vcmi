# BattleHex

Represents a single tile on the battlefield gridallows computing geometric relationships (distance, neighbours, line-of-sight) that don't rely on current state of a battlefield

### isValid

Returns true if this hex value identifies existing battlefield position.

- returns `boolean`

### isAvailable

Returns true if the hex is on the battlefield and is not located in the first or last column, which are reserved for back tile of war machines and are not accessible for regular movement. NOTE: does NOT checks whether hex is blocked by obstacles or units

- returns `boolean`

### getClosestTile

Searches provided array for hex that is closest to current one. If multiple equidistant hexes are found, this function will prefer hex closest to specified battle side.

- param `side`: [`BattleSide`](BattleSide.md) — Tie-breaker side — closer to this side wins when several candidates are equidistant.
- param `hexes`: [`BattleHexArray`](BattleHexArray.md) — Candidate hexes to search through.

- returns [`BattleHex`](BattleHex.md)

### copyToNorthWest

Returns the neighbouring hex one step north-west.

- returns [`BattleHex`](BattleHex.md)

### copyToNorthEast

Returns the neighbouring hex one step north-east.

- returns [`BattleHex`](BattleHex.md)

### copyToEast

Returns the neighbouring hex one step east.

- returns [`BattleHex`](BattleHex.md)

### copyToSouthEast

Returns the neighbouring hex one step south-east.

- returns [`BattleHex`](BattleHex.md)

### copyToSouthWest

Returns the neighbouring hex one step south-west.

- returns [`BattleHex`](BattleHex.md)

### copyToWest

Returns the neighbouring hex one step west.

- returns [`BattleHex`](BattleHex.md)
