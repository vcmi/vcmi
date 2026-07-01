# SpellObstacleDescriptor

Configuration for a battlefield obstacle that scripts hand to ServerCallback `addObstacle`. Captures anchor position, owning spell, caster stats, behavior flags, presentation assets, and an obstacle hex footprint.

### pos

Anchor BattleHex of the obstacle (its primary position). Usually located in bottom-left corner of the obstacle

- type: [`BattleHex`](BattleHex.md)

### obstacleType

Obstacle category — usual, absolute, spell-created, moat. See the ObstacleType enum.

- type: [`ObstacleType`](ObstacleType.md)

### spell

Spell that created the obstacle. Used for dispel and damage-source attribution. Only for spell-created obstacles

- type: [`Spell`](Spell.md)

### turnsRemaining

How many turns the obstacle persists. -1 means permanent for the battle.

- type: `integer`

### casterSpellPower

Spell power of the caster at the moment of creation; feeds damage formulas.

- type: `integer`

### spellLevel

Spell skill level (0–3) the obstacle was cast at.

- type: `integer`

### casterSide

Which battle side cast it; relevant for native-visibility and friendly-fire rules.

- type: [`BattleSide`](BattleSide.md)

### minimalDamage

Floor for the damage the obstacle inflicts on trigger.

- type: `integer`

### hidden

If true, the obstacle is invisible to the opposing side until triggered.

- type: `boolean`

### passable

If true, units may step onto the obstacle's hexes (e.g. trap obstacles).

- type: `boolean`

### trap

If true, behaves as a trap: triggers on enter rather than blocking movement.

- type: `boolean`

### removeOnTrigger

If true, the obstacle disappears after being triggered once.

- type: `boolean`

### nativeVisible

If true, the caster's side always sees the obstacle even when `hidden` is set.

- type: `boolean`

### trigger

Script-side identifier of the trigger spell (looked up by the spell mechanics).

- type: `string`

### appearSound

Sound effect played when the obstacle appears.

- type: `string`

### appearAnimation

Animation played when the obstacle appears.

- type: `string`

### animation

Looping animation shown while the obstacle is on the battlefield.

- type: `string`

### customSize

Optional list of BattleHex values defining a multi-hex footprint.

- type: [`BattleHex[]`](BattleHex.md)
