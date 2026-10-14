# Unit

Represents a creature stack participating in the current battle. Provides access to the live combat state — position, owner, current health, applied bonuses, ability checks. To stage modifications, copy into a UnitState, edit it, then commit via server.

### getBonuses

Returns the bonuses of the bearer that match the filter. Say as much as the filter can express, since that is also what the engine can cache; narrow whatever is left with `BonusList:filter`.

- param `filter`: [`BonusFilter`](BonusFilter.md) — Which bonuses to collect. An empty filter collects every one of them.

- returns [`BonusList`](BonusList.md) — Bonuses of the bearer the filter describes.

### getBonusesValue

Returns what the matching bonuses are worth together. Not a plain sum - percentages, independent floors and ceilings combine by the rules of the engine. Prefer this over adding up `getBonuses` where possible.

- param `filter`: [`BonusFilter`](BonusFilter.md) — Which bonuses to count. An empty filter counts every one of them.

- returns `integer` — Value of the matching bonuses taken together.

### hasBonuses

True if the bearer carries a bonus the filter describes. Prefer this over testing the size of `getBonuses`, which hands the whole list over to the script to answer a question the engine can answer on its own.

- param `filter`: [`BonusFilter`](BonusFilter.md) — Which bonuses to look for. An empty filter asks whether the bearer has any at all.

- returns `boolean` — True if the bearer carries at least one matching bonus.

### getMinDamage

Returns the minimum damage one creature in the stack will deal.

- param `ranged`: `boolean` — True for ranged attack value, false for melee.

- returns `integer`

### getMaxDamage

Returns the maximum damage one creature in the stack will deal.

- param `ranged`: `boolean` — True for ranged attack value, false for melee.

- returns `integer`

### getAttack

Returns the creature's attack stat.

- param `ranged`: `boolean` — True for ranged attack value, false for melee.

- returns `integer`

### getDefense

Returns the creature's defense stat.

- param `ranged`: `boolean` — True for defense against ranged attacks, false for defense against melee.

- returns `integer`

### isAlive

True if the stack has at least one alive creature.

- returns `boolean`

### isClone

True if this stack is a clone produced by a Clone spell.

- returns `boolean`

### hasClone

True if this stack has an alive clone summoned by a Clone spell.

- returns `boolean`

### isDead

True if the stack has no remaining alive creatures.

- returns `boolean`

### isGhost

True if the stack was completely removed from the battlefield including its corpse.

- returns `boolean`

### isValidTarget

True if the stack can be targeted by spells / attacks.

- param `allowDead`: `boolean` — Pass true to count dead but resurrectable stacks as valid targets.

- returns `boolean`

### isInvincible

True if the stack has invincibility (cannot be damaged or killed).

- returns `boolean`

### hasAbsoluteImmunity

True if the unit is absolutely immune to the given spell.

- param `spell`: [`Spell`](Spell.md) — Spell to test absolute immunity against.

- returns `boolean`

### isSummoned

True if the stack was summoned during battle (e.g. by Summon Elementals).

- returns `boolean`

### isLiving

True if the stack is a living creature - not undead, not a golem-like non-living unit.

- returns `boolean`

### getOwner

Returns the player color controlling this unit.

- returns `integer`

### getSlot

Returns the army slot in the army this unit occupies. NOTE: All summoned units share the same slot

- returns `integer`

### getSide

Returns the battle side (attacker or defender) this unit belongs to.

- returns [`BattleSide`](BattleSide.md)

### getPosition

Returns the battlefield hex occupied by the unit, or front hex for double-wide units

- returns [`BattleHex`](BattleHex.md)

### getTotalHealth

Returns the total hit points across all creatures in the stack, including dead.

- returns `integer`

### getAvailableHealth

Returns the current hit points of living creatures of this unit.

- returns `integer`

### getCount

Returns the number of creatures currently alive in the stack.

- returns `integer`

### getFirstHPleft

Returns the health left of the first creature in the unit stack.

- returns `integer`

### isShooter

True if the stack can shoot in general, even if out of ammo. See canShoot to check if unit can shoot right now.

- returns `boolean`

### isTurret

True if the stack is one of the towers of a besieged town.

- returns `boolean`

### getTurretPart

Which of the three towers of a besieged town this stack is.

- returns `string?` — "keep", "upper" or "lower"; nil when the stack is no tower.

### getMaxHealth

Returns the maximum hit points of a single creature in the stack.

- returns `integer`

### coversPos

True if the unit currently covers the given hex (accounts for double-wide creatures).

- param `position`: [`BattleHex`](BattleHex.md) — Battlefield hex to test against the unit's footprint.

- returns `boolean`

### getCreature

Returns the Creature type of the units in this stack.

- returns [`Creature`](Creature.md)

### getBaseAmount

Returns the initial number of creatures this stack had at battle start.

- returns `integer`

### getHexes

Returns the list of hexes currently occupied by the unit.

- returns [`BattleHexArray`](BattleHexArray.md)

### getSurroundingHexes

Returns the hexes adjacent to the unit - six for a single-hex unit, eight for a double-wide one.

- returns [`BattleHexArray`](BattleHexArray.md)

### copy

Returns a copy of the unit's state allowing copying or changing this unit via server calls.

- returns [`UnitState`](UnitState.md)

### creatureLevel

Returns the creature level (1..7) of the unit's type.

- returns `integer`

### getLevel

Returns the level of the stack itself, which for a commander is its own level rather than the tier of its creature. Use `creatureLevel` when the creature type is what matters.

- returns `integer`

### unitID

DEPRECATED. Returns the unit's internal numeric identifier.

- returns `integer`
