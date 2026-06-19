# Server

The authoritative-side mutation interface. Available only to scripts running on the server: spawn or remove battle units, move them, deal damage, alter bonuses, drop obstacles, append to the combat log, draw from the seeded RNG. Every call emits a network pack so clients receive the resulting state change.

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
- param `attacker`: [`Unit`](Unit.md) — Unit performing the catapult attack.
- param `attackedPart`: [`WallPart`](WallPart.md) — Wall section to attack.
- param `damageDealt`: `integer` — Damage to apply to the wall section.

### rngInt

Returns a server-side random integer in the inclusive range [low, high].

- param `low`: `integer` — Inclusive lower bound.
- param `high`: `integer` — Inclusive upper bound.

- returns `integer` — Random integer in [low, high].
