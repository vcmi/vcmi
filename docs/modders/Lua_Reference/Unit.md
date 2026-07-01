# Unit

Represents a creature stack participating in the current battle. Provides access to the live combat state — position, owner, current health, applied bonuses, ability checks. To stage modifications, copy into a UnitState, edit it, then commit via server.

### getBonuses

Returns all bonuses affecting the bearer for which the predicate returns true.

- param `predicate`: `fun(b: Bonus): boolean` — Selector — called for each bonus on the bearer; bonus is kept when it returns true.

- returns [`BonusList`](BonusList.md) — Bonuses for which the predicate returned true.

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

### getOwner

Returns the player color controlling this unit.

- returns `integer`

### getSlot

Returns the army slot in the army this unit occupies. NOTE: All summoned units share the same slot

- returns `integer`

### unitSide

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

### copy

Returns a copy of the unit's state allowing copying or changing this unit via server calls.

- returns [`UnitState`](UnitState.md)

### creatureLevel

Returns the creature level (1..7) of the unit's type.

- returns `integer`

### unitID

DEPRECATED. Returns the unit's internal numeric identifier.

- returns `integer`
