# HeroInstance

A hero placed on the adventure map. Provides access to such information as owner, type (Orrin, Kyrre, Astral...), position on map, artifacts, and secondary skills, army composition, primary skills, and all bonuses affecting hero.

### getBonuses

Returns all bonuses affecting the bearer for which the predicate returns true.

- param `predicate`: `fun(b: Bonus): boolean` — Selector — called for each bonus on the bearer; bonus is kept when it returns true.

- returns [`BonusList`](BonusList.md) — Bonuses for which the predicate returned true.

### getStack

Returns the stack instance in the given army slot, or nil if the slot is empty.

- param `slot`: `integer` — Army slot to query (1-based).

- returns [`StackInstance`](StackInstance.md)

### getOwner

Returns the player color that owns this hero.

- returns `integer`

### getNameTextID

Returns the text ID of the hero's name.

- returns `string`

### isMale

True if the hero's gender is male.

- returns `boolean`

### isFemale

True if the hero's gender is female.

- returns `boolean`
