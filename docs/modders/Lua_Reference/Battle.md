# Battle

Battlefield query interface. The handle scripts receive whenever they are passed a battle context — enumerate units and obstacles, test hex accessibility and shooting penalties, inspect wall state on siege maps. Read-only; mutations go through Server.

### getTacticDistance

Returns the available tactic phase distance, or 0 if the tactic phase has ended.

- returns `integer`

### getAvailableHex

Returns an empty hex next to desired location that the creature can be placed on.

- param `creature`: [`Creature`](Creature.md) — Creature template whose footprint is being placed.
- param `side`: [`BattleSide`](BattleSide.md) — Side whose deployment area to search.
- param `hex`: [`BattleHex?`](BattleHex.md) — Preferred origin hex; nil falls back to a side-appropriate default.

- returns [`BattleHex`](BattleHex.md) — Empty hex closest to the desired location that fits the creature, or INVALID if none.

### getUnitsIf

Returns all units for which the predicate returns true.

- param `predicate`: `fun(u: Unit): boolean` — Selector — called for each unit on the battlefield; unit is kept when it returns true.

- returns [`Unit[]`](Unit.md) — Units for which the predicate returned true.

### isAccessibleForUnit

True if the given hex is reachable by the given unit either on current turn or on any future turns.

- param `unit`: [`Unit`](Unit.md) — Unit whose movement model is consulted.
- param `hex`: [`BattleHex`](BattleHex.md) — Hex to test for reachability.

- returns `boolean`

### hasPenaltyOnLine

True if a ranged attack along this line crosses a wall or moat (per the flags).

- param `from`: [`BattleHex`](BattleHex.md) — Origin hex of the ranged attack.
- param `dest`: [`BattleHex`](BattleHex.md) — Target hex of the ranged attack.
- param `checkWall`: `boolean` — Pass true to count crossing a wall as a penalty source.
- param `checkMoat`: `boolean` — Pass true to count crossing a moat as a penalty source.

- returns `boolean`

### getUnitByPos

Returns the unit covering the given hex, or nil.

- param `hex`: [`BattleHex`](BattleHex.md) — Hex to inspect for a unit.
- param `onlyAlive`: `boolean` — Pass true to skip dead-but-resurrectable stacks.

- returns [`Unit`](Unit.md)

### getAllObstacles

Returns all obstacles on the battlefield.

- returns [`Obstacle[]`](Obstacle.md)

### getObstaclesOnPos

Returns the obstacles on the given hex.

- param `hex`: [`BattleHex`](BattleHex.md) — Hex whose obstacles are queried.
- param `onlyBlocking`: `boolean` — Pass true to limit results to obstacles that block movement.

- returns [`Obstacle[]`](Obstacle.md)

### hasFortifications

True if the battle is a siege with fortifications present.

- returns `boolean`

### hasMoat

True if the battlefield has a moat.

- returns `boolean`

### hasNativeStack

True if the given side has at least one native-terrain stack.

- param `side`: [`BattleSide`](BattleSide.md) — Battle side to inspect (attacker or defender).

- returns `boolean`

### getAllPossibleHexes

Returns every valid battlefield hex.

- returns [`BattleHexArray`](BattleHexArray.md)

### getWallState

Returns the current state of the given wall section, or nil if absent.

- param `part`: [`WallPart`](WallPart.md) — Wall section to query.

- returns `integer?`

### isWallPartAttackable

True if the given wall section can be targeted by an attack.

- param `part`: [`WallPart`](WallPart.md) — Wall section to test.

- returns `boolean`

### wallPartToBattleHex

Returns the battle hex corresponding to the given wall section.

- param `part`: [`WallPart`](WallPart.md) — Wall section to look up.

- returns [`BattleHex`](BattleHex.md)

### hexToWallPart

Returns the wall section corresponding to the given battle hex.

- param `hex`: [`BattleHex`](BattleHex.md) — Hex to look up.

- returns [`WallPart`](WallPart.md)

### getTowerShooterHex

Returns the hex used by the tower shooter for the given wall section.

- param `part`: [`WallPart`](WallPart.md) — Wall section whose tower-shooter hex is queried.

- returns [`BattleHex`](BattleHex.md)
