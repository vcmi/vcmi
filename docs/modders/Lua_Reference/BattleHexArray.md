# BattleHexArray

A list of BattleHex values. Used wherever the engine returns or accepts a set of tiles (reachable hexes, area-of-effect, obstacle footprint, …). Supports indexed access, insertion, and erasure; iteration order matches insertion order

### insert

Adds the given hex to the list (no-op if already present).

- param `hex`: [`BattleHex`](BattleHex.md) — Hex to insert.

### erase

Removes the given hex from the list (no-op if absent).

- param `hex`: [`BattleHex`](BattleHex.md) — Hex to remove.

### contains

True if the list contains the given hex.

- param `hex`: [`BattleHex`](BattleHex.md) — Hex to test for membership.

- returns `boolean`

### size

Returns the number of unique hexes stored in the list.

- returns `integer`

### at

Returns the hex at the given 1-based index.

- param `index`: `integer` — 1-based position of the hex to fetch.

- returns [`BattleHex`](BattleHex.md)
