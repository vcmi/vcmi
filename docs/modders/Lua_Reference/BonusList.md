# BonusList

A collection of Bonus values returned by `getBonuses(...)`. Use `size()` and `getBonus(index)` to iterate. A copy of the engine's internal list at the moment of the call — changes to holder afterwards will not affect this snapshot.

### size

Returns the number of bonuses in this list.

- returns `integer`

### getBonus

Returns the bonus at the given 1-based index. Throws if the index is out of range.

- param `index`: `integer` — 1-based position of the bonus to fetch.

- returns [`Bonus`](Bonus.md) — Bonus stored at the given position.
