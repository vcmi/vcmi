# Enums

The global `ENUM` table — string->integer constants scripts use to avoid magic numbers. Each field below is a named group; the runtime value is a Lua table whose keys are the constant names.

### HealLevel

Outcome categories for the heal action: plain heal / resurrect / overheal.

- type: [`HealLevel`](HealLevel.md)

### HealPower

Persistence of healing effects: one-battle vs permanent.

- type: [`HealPower`](HealPower.md)

### SpellCastProblem

Error codes returned from spell-cast validation.

- type: [`SpellCastProblem`](SpellCastProblem.md)

### AimType

Targeting modes a spell can require: none, creature, obstacle, location.

- type: [`AimType`](AimType.md)

### BonusDuration

Lifetime selectors used by Bonus / BonusDescriptor `duration`.

- type: [`BonusDuration`](BonusDuration.md)

### BonusSource

Origin classes used by Bonus / BonusDescriptor `sourceType`.

- type: [`BonusSource`](BonusSource.md)

### BonusValueType

Combination rules used by Bonus / BonusDescriptor `valueType`.

- type: [`BonusValueType`](BonusValueType.md)

### ObstacleType

Obstacle categories used by SpellObstacleDescriptor `obstacleType`.

- type: [`ObstacleType`](ObstacleType.md)

### WallPart

Town-wall sections referenced by siege APIs and `catapultAttack`.

- type: [`WallPart`](WallPart.md)

### BattleSide

Battlefield side identifiers: none / attacker / defender.

- type: [`BattleSide`](BattleSide.md)
