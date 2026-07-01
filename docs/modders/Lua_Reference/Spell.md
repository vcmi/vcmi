# Spell

A spell definition (Fire Bolt, Slow, Town Portal, …). Identifies kind and metadata (schools, level). Cast resolution and per-cast state live on SpellMechanics.

### getJsonKey

Returns the JSON key (mod-scoped identifier) of this entity.

- returns `string`

### getNameTextID

Returns the text ID of the spell name.

- returns `string`

### isAdventure

True if the spell can only be cast on the adventure map.

- returns `boolean`

### isCombat

True if the spell can only be cast during combat.

- returns `boolean`

### isCreatureAbility

True if the spell is a creature's innate ability rather than a learnable spell.

- returns `boolean`

### isPositive

True if the spell is classified as beneficial to its target.

- returns `boolean`

### isNegative

True if the spell is classified as harmful to its target.

- returns `boolean`

### isNeutral

True if the spell is classified as neutral (neither positive nor negative).

- returns `boolean`

### isDamage

True if the spell deals direct damage.

- returns `boolean`

### isOffensive

True if the spell is offensive (damage / curse-type effects).

- returns `boolean`

### isSpecial

True if the spell is marked as special (e.g. cannot be obtained from common sources).

- returns `boolean`

### isPersistent

True if the spell's effect persists and can't be dispelled normally.

- returns `boolean`

### isMagical

True if the spell is magical in nature (as opposed to a non-magical ability).

- returns `boolean`

### getCost

Returns the mana cost of the spell at the requested skill level.

- param `skillLevel`: `integer` — Mastery level used to look up the cost (0=basic, up to 3=expert).

- returns `integer`

### getBasePower

Returns the spell's base power before caster bonuses.

- returns `integer`

### getLevelPower

Returns the spell's per-level power bonus.

- param `skillLevel`: `integer` — Mastery level used to look up the power bonus (0=basic, up to 3=expert).

- returns `integer`

### getSchools

Returns the list of magic schools the spell belongs to.

- returns [`SpellSchool[]`](SpellSchool.md)
