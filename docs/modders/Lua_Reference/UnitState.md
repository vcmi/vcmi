# UnitState

Editable copy of a battle Unit. Obtained via `Unit:copy()`; scripts edit fields (position, defending, clone, summoned, …) and inflict damage locally, then hand the result to ServerCallback `changeUnit` or `addUnit` to broadcast the update through a battle pack.

### getMinDamage

Returns the minimum damage one creature will deal.

- param `ranged`: `boolean` — True for ranged attack value, false for melee.

- returns `integer`

### getMaxDamage

Returns the maximum damage one creature will deal.

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

True if this stack is a clone produced by the Clone spell.

- returns `boolean`

### isDead

True if the stack has no remaining alive creatures.

- returns `boolean`

### isGhost

True if the stack was completely removed from the battlefield including its corpse.

- returns `boolean`

### isValidTarget

True if the stack can be targeted.

- param `allowDead`: `boolean` — Pass true to count dead but resurrectable stacks as valid targets.

- returns `boolean`

### isInvincible

True if the stack has invincibility.

- returns `boolean`

### isSummoned

True if the stack was summoned during battle.

- returns `boolean`

### getOwner

Returns the player color controlling this unit.

- returns `integer`

### getSlot

Returns the army slot in the army this unit occupies. NOTE: All summoned units share the same slot

- returns `integer`

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

Returns the maximum hit points of a single creature.

- returns `integer`

### coversPos

True if the unit currently covers the given hex.

- param `position`: [`BattleHex`](BattleHex.md) — Battlefield hex to test against the unit's footprint.

- returns `boolean`

### getBaseAmount

Returns the initial number of creatures this stack had at battle start.

- returns `integer`

### hasAbsoluteImmunity

True if the unit is absolutely immune to the given spell.

- param `spell`: [`Spell`](Spell.md) — Spell to test absolute immunity against.

- returns `boolean`

### getCreature

Returns the Creature type of the units in this stack.

- returns [`Creature`](Creature.md)

### getHexes

Returns the list of hexes currently occupied by the unit.

- returns [`BattleHexArray`](BattleHexArray.md)

### setDefending

Sets whether unit has defended in this round.

- param `value`: `boolean` — New defending flag.

### setCloned

Marks the stack as a clone.

- param `value`: `boolean` — New cloned flag.

### setDrainedMana

Marks that the unit has had drained mana from enemy hero this turn.

- param `value`: `boolean` — New drained-mana flag.

### setFear

Sets the feared status (skips the unit's current turn).

- param `value`: `boolean` — New feared flag — when true the unit will skip its next turn.

### setHadMorale

Marks that the unit has already triggered a morale bonus this turn.

- param `value`: `boolean` — New had-morale flag.

### setCastSpellThisTurn

Marks that the unit has cast a spell this turn.

- param `value`: `boolean` — New cast-spell-this-turn flag.

### setGhost

Sets the ghost state removing them from the battle

- param `value`: `boolean` — New ghost flag — when true the unit becomes invisible to the battle.

### setGhostPending

Marks the unit as transitioning to the ghost state.

- param `value`: `boolean` — New ghost-pending flag.

### setMoved

Marks that the unit has moved this round.

- param `value`: `boolean` — New moved-this-round flag.

### setSummoned

Marks the unit as summoned during combat.

- param `value`: `boolean` — New summoned flag.

### setWaiting

Marks that the unit is currently waiting.

- param `value`: `boolean` — New waiting flag.

### setWaitedThisTurn

Marks that the unit has already used its wait this turn.

- param `value`: `boolean` — New waited-this-turn flag.

### setPosition

Moves the unit to the given hex.

- param `hex`: [`BattleHex`](BattleHex.md) — Destination hex for the unit.

### setClone

Links this stack to a clone unit produced by a Clone spell.

- param `unit`: [`Unit`](Unit.md) — Unit instance to register as this stack's clone.

### damage

Deals damage to the stack, clamped to available health.

- param `amount`: `integer` — Damage to apply; will be clamped to remaining health.

- returns `integer` — Damage value actually applied (may be less than requested if the stack was killed).

### heal

Heals the stack. Returns the healed hit points and the resurrected creature count, if any.

- param `amount`: `integer` — Hit points to restore.
- param `level`: [`HealLevel`](HealLevel.md) — Heal tier (heal, resurrect, overheal).
- param `power`: [`HealPower`](HealPower.md) — Persistence — one-battle vs permanent.

- returns `integer, integer` — Healed hit points, and the count of creatures resurrected (zero if none).
