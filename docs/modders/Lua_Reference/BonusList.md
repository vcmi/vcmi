# BonusList

A collection of Bonus values returned by `getBonuses(...)`. Use `size()` and `getBonus(index)` to iterate. A copy of the engine's internal list at the moment of the call — changes to holder afterwards will not affect this snapshot.

### size

Returns the number of bonuses in this list.

- returns `integer`

### totalValue

Computes total value of bonuses in the list, accounting for bonus value types

- returns `integer`

### filter

Returns the bonuses of this list the predicate accepts. Use to narrow a list down before `totalValue`, which combines what is left by the rules of the engine.

- param `predicate`: `fun(b: Bonus): boolean` — Selector — called for each bonus of the list; bonus is kept when it returns true.

- returns [`BonusList`](BonusList.md) — Bonuses for which the predicate returned true.

### getBonus

Returns the bonus at the given 1-based index. Aborts the script if the index is out of range.

- param `index`: `integer` — 1-based position of the bonus to fetch.

- returns [`Bonus`](Bonus.md) — Bonus stored at the given position.
