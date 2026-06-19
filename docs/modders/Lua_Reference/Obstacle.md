# Obstacle

A battlefield obstacle (static map decoration, moat tile, or spell-created hazard). Exposed read-only — to add or remove obstacles use the `server:addObstacle` or `server:removeObstacle` methods.

### getObstacleType

Returns the obstacle category: usual, absolute, moat, or spell-created.

- returns [`ObstacleType`](ObstacleType.md)

### getPosition

Returns the hex that serves as anchor of the obstacle, usually - located in bottom-left corner of the obstacle

- returns [`BattleHex`](BattleHex.md)

### getSpell

Returns the Spell that created this obstacle, or nil for non-spell obstacles.

- returns [`Spell`](Spell.md)
