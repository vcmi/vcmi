# BattleServer

The authoritative battle mutation interface. Available only to scripts running on the server: spawn or remove battle units, move them, deal damage, alter bonuses, drop obstacles, append to the combat log, draw from the seeded RNG. Every call emits a network pack so clients receive the resulting state change.

### addUnit

Spawns a new battle unit described by the given UnitInfo. Returns the created unit.

- param `battle`: [`Battle`](Battle.md) — Battle in which the unit is created.
- param `info`: `UnitInfo` — Descriptor of the unit to spawn (creature type, side, position, ...).

- returns [`Unit`](Unit.md)

### healUnit

Heals the given unit by the provided amount of health points.

- param `battle`: [`Battle`](Battle.md) — Battle in which the unit is healed.
- param `unit`: [`Unit`](Unit.md) — Target unit.
- param `amount`: `integer` — Hit points to restore.
- param `level`: [`HealLevel`](HealLevel.md) — Heal tier (heal / resurrect / overheal).
- param `power`: [`HealPower`](HealPower.md) — Persistence — one-battle vs permanent.

- returns `integer, integer` — Healed hit points, and the count of creatures resurrected.

### changeUnit

Applies a UnitState mutation to the unit, optionally adjusting current health.

- param `battle`: [`Battle`](Battle.md) — Battle in which the unit is modified.
- param `unitState`: [`UnitState`](UnitState.md) — New unit state to apply (returned by `Unit:copy`).
- param `healthDelta`: `integer?` — Optional health delta — positive heals, negative damages.

### damageUnit

Damages the unit, returning the actual damage dealt and the number of killed creatures.

- param `battle`: [`Battle`](Battle.md) — Battle in which damage is dealt.
- param `unit`: [`Unit`](Unit.md) — Target unit.
- param `damage`: `integer` — Damage points to deal (will be clamped to remaining health).

- returns `integer, integer` — Damage actually dealt, and the count of killed creatures.

### removeUnit

Removes the unit or its corpse from the battlefield.

- param `battle`: [`Battle`](Battle.md) — Battle the unit belongs to.
- param `unit`: [`Unit`](Unit.md) — Unit (alive or as corpse) to remove.

### removeObstacle

Removes the given obstacle from the battlefield.

- param `battle`: [`Battle`](Battle.md) — Battle the obstacle belongs to.
- param `obstacle`: [`Obstacle`](Obstacle.md) — Obstacle to remove.

### moveUnit

Moves the unit to the destination hex.

- param `battle`: [`Battle`](Battle.md) — Battle in which the unit is moved.
- param `unit`: [`Unit`](Unit.md) — Unit to move.
- param `destination`: [`BattleHex`](BattleHex.md) — Target hex of the move.
- param `isTeleport`: `boolean` — Pass true to use teleport semantics (no path walk).

### appendLog

Appends a formatted log entry to the battle log.

- param `battle`: [`Battle`](Battle.md) — Battle whose log is being appended.
- param `message`: [`MetaString`](MetaString.md) — Formatted log line (use `MetaString`).

### describeChanges

Returns whether netpack changes should be described in the battle log.

- returns `boolean`

### removeUnitBonuses

Removes the listed bonuses from the unit.

- param `battle`: [`Battle`](Battle.md) — Battle the unit belongs to.
- param `unit`: [`Unit`](Unit.md) — Unit whose bonuses are removed.
- param `bonusList`: [`BonusList`](BonusList.md) — Bonuses to remove from the unit.

### addUnitBonus

Adds a bonus described by the descriptor to the unit.

- param `battle`: [`Battle`](Battle.md) — Battle the unit belongs to.
- param `unit`: [`Unit`](Unit.md) — Unit to add the bonus to.
- param `descriptor`: [`BonusDescriptor`](BonusDescriptor.md) — BonusDescriptor that creates the bonus.
- param `cumulative`: `boolean` — Pass true to stack with existing same-source bonus, false to update in-place.

### addBattleBonus

Adds a bonus to the battle-wide bonus set.

- param `battle`: [`Battle`](Battle.md) — Battle to attach the bonus to.
- param `descriptor`: [`BonusDescriptor`](BonusDescriptor.md) — BonusDescriptor that creates the bonus.

### addObstacle

Creates a new obstacle described by the descriptor on the battlefield.

- param `battle`: [`Battle`](Battle.md) — Battle the obstacle is placed in.
- param `descriptor`: [`SpellObstacleDescriptor`](SpellObstacleDescriptor.md) — SpellObstacleDescriptor describing the obstacle to create.

### catapultAttack

Performs a catapult attack against the given wall section, dealing the supplied damage.

- param `battle`: [`Battle`](Battle.md) — Battle in which the catapult attack happens.
- param `attacker`: [`Unit`](Unit.md) — Unit performing the catapult attack, or nil for spell-caused attacks.
- param `attackedPart`: [`WallPart`](WallPart.md) — Wall section to attack.
- param `damageDealt`: `integer` — Damage to apply to the wall section.

### rngInt

Returns a server-side random integer in the inclusive range [low, high].

- param `low`: `integer` — Inclusive lower bound.
- param `high`: `integer` — Inclusive upper bound.

- returns `integer` — Random integer in [low, high].

### rngBinomial

Rolls the same chance many times over and returns how many succeeded. Use this rather than a loop over `rngInt` for abilities that roll once per creature in a stack, which can number in the thousands.

- param `trials`: `integer` — How many independent chances are rolled.
- param `chance`: `number` — Chance of each one succeeding, from 0 to 1.

- returns `integer` — How many of them succeeded.

### rollCombatAbility

Rolls a chance-based combat ability. Use this rather than `rngInt` for abilities that trigger with a percentage chance - it draws from the per-army biased sequence

- param `battle`: [`Battle`](Battle.md) — Battle the acting unit fights in.
- param `actor`: [`Unit`](Unit.md) — Unit whose ability is being rolled.
- param `percentageChance`: `integer` — Chance to succeed, in percent.

- returns `boolean` — True if the ability triggers.

### castSpell

Casts a spell as a passive ability of the caster: announces it so that clients play its animation and sound, filters the targets through the spell's own immunity rules, and applies its effects with the given magnitude. Casting this way never fires a combat event, so a script that casts cannot re-enter itself.

- param `battle`: [`Battle`](Battle.md) — Battle the spell is cast in.
- param `caster`: [`Unit`](Unit.md) — Unit whose ability casts the spell.
- param `spell`: [`Spell`](Spell.md) — Spell to cast.
- param `target`: [`Unit[]`](Unit.md) — Units the spell is aimed at.
- param `effectValue`: `integer` — Magnitude handed to the spell's effects, for spells that take one.

### applySpellEffects

Applies the effects of a spell to the given units, and nothing else. Unlike casting the spell, the target list is used as given rather than expanded through the spell's range, magic resistance and magic mirror are not rolled, countering effects are not removed, and no spell animation or battle log entry is produced. Use it for abilities that behave as if the spell were already in effect.

- param `battle`: [`Battle`](Battle.md) — Battle the spell is applied in.
- param `caster`: [`Unit`](Unit.md) — Unit acting as the caster. Its stats scale the effects.
- param `spell`: [`Spell`](Spell.md) — Spell whose effects are applied.
- param `target`: [`Unit[]`](Unit.md) — Units to affect.
- param `spellLevel`: `integer` — Mastery level the effects are applied at.
- param `effectDuration`: `integer` — How many turns timed effects of the spell last.
- param `ignoreImmunity`: `boolean` — Pass true to affect units that are immune to the spell.

### showBattleAnimation

Plays a one-shot animation on the battlefield. Changes no game state, so use it to give a visual to a change the script made itself. The animation plays after whatever is currently animating has finished, rather than overlapping it. A deferred animation instead plays together with the animations of the next change the script makes, e.g. the flinch of a unit it damages right after - and is dropped if no such change follows.

- param `battle`: [`Battle`](Battle.md) — Battle the animation is played in.
- param `target`: `Destination[]` — Units and hexes the animation is played on, one copy each.
- param `animation`: `string` — Resource name of the animation to play, e.g. `SP06_`.
- param `sound`: `string` — Resource name of the sound to play alongside it, or an empty string for none.
- param `transparency`: `number` — Opacity of the animation, from 0 for invisible to 1 for opaque.
- param `deferred`: `boolean?` — Pass true to hold the animation back until the next change the script makes, so that both play at once.

### refreshBattleUnits

Makes the client play back pending unit changes. Needed after adding or removing units outside of an attack, which otherwise produce no animation.

- param `battle`: [`Battle`](Battle.md) — Battle whose units were changed.
