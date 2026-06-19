# Bonus

A single effect contributing to target abilities (e.g. +5 attack, immunity to fire spells). Carries type, value, source, duration, stacking key. Scripts read bonuses through `getBonuses(...)` and construct new ones via addUnitBonus or addBattleBonus in ServerCallback.

### getType

Returns the bonus type as its JSON key.

- returns `string`

### getVal

Returns the integer magnitude of the bonus.

- returns `integer`

### getSubtype

Returns the bonus subtype as a JSON key (the meaning depends on getType).

- returns `string`

### getSourceID

Returns the JSON key identifying the entity that granted this bonus.

- returns `string`

### getSource

Returns the source category (artifact, creature ability, spell, ...) of the bonus.

- returns [`BonusSource`](BonusSource.md)

### getDuration

Returns the list of duration flags currently set on the bonus.

- returns [`BonusDuration[]`](BonusDuration.md)

### getValType

Returns how the value combines with other bonuses (additive, percent, base number, ...).

- returns [`BonusValueType`](BonusValueType.md)

### getStacking

Returns the stacking key — bonuses with the same key from the same source do not stack.

- returns `string`

### getTurnsRemain

Returns the remaining turns until the bonus expires (0 if not turn-limited, only active if duration is set accordingly).

- returns `integer`

### isHidden

True if the bonus is hidden from the player's interface display.

- returns `boolean`

### getParametersAsNumber

Returns the bonus's extra parameters encoded as a single integer (0 if none).

- returns `integer`
