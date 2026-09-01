# Creature

A creature type from the database (e.g. "pikeman"). Carries base stats, faction, recruitment cost, and the bonuses every instance starts with. Concrete battlefield or army-slot instances are Unit and StackInstance respectively.

### getJsonKey

Returns the JSON key (mod-scoped identifier) of this entity.

- returns `string`

### getMaxHealth

Returns the base maximum hit points of a single creature of this type. Equivalent to getBaseHitPoints.

- returns `integer`

### getNameTextID

Select either the singular or plural text ID for the requested amount.

- param `amount`: `integer` — Stack size; passes through to the singular form when equal to 1, plural otherwise.

- returns `string`

### getMapAmountMin

Returns the minimum stack size for this creature when generated on the adventure map.

- returns `integer`

### getMapAmountMax

Returns the maximum stack size for this creature when generated on the adventure map.

- returns `integer`

### getAIValue

Returns the AI value used for army strength calculations.

- returns `integer`

### getFightValue

Returns the combat power value of one creature of this type.

- returns `integer`

### getLevel

Returns the creature level, usually in (1..7) range.

- returns `integer`

### getGrowth

Returns the base weekly growth rate for this creature in town dwellings.

- returns `integer`

### getHorde

Returns the extra growth granted by the horde building (0 if none).

- returns `integer`

### getRecruitCost

Returns the recruitment cost in the specified resource.

- param `resourceID`: `integer` — JSON key of the resource whose cost is being queried (e.g. `gold`, `ore`).

- returns `integer`

### isDoubleWide

True if the creature occupies two hexes on the battlefield.

- returns `boolean`
