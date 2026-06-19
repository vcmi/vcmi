# SpellMechanics

Per-cast spell context: which Spell is being cast, who is casting it, at what power and level. Lua spell scripts receive this as the entry point to their cast logic and consult it for caster identity, target validation, and damage formulas.

### isPositive

True if the spell mechanics classify this cast as a positive effect.

- returns `boolean`

### isNegative

True if the spell mechanics classify this cast as a negative effect.

- returns `boolean`

### isSmart

True if the spell only affects friendly or enemy targets (vs. anyone in range).

- returns `boolean`

### isMassive

True if the spell targets all valid targets simultaneously.

- returns `boolean`

### alwaysHitFirstTarget

True if the first selected target is always considered hit, ignoring magic resistance.

- returns `boolean`

### wouldResist

Returns true if the given target would resist this cast.

- param `target`: [`Unit`](Unit.md) — Unit whose resistance is being tested.

- returns `boolean`

### getEffectLevel

Returns the effective mastery level used for the spell's magnitude.

- returns `integer`

### getRangeLevel

Returns the effective mastery level used for the spell's range.

- returns `integer`

### getEffectPower

Returns the effective spell power applied to the magnitude calculation.

- returns `integer`

### getEffectDuration

Returns the effect duration in turns.

- returns `integer`

### getEffectValue

Returns the computed effect value (e.g. damage / health amount).

- returns `integer`

### getCasterColor

Returns the player color of the caster.

- returns `integer`

### getCasterSide

Returns the battle side of the caster.

- returns [`BattleSide`](BattleSide.md)

### getHeroCaster

Returns the hero performing the cast, or nil if cast by a unit.

- returns [`HeroInstance`](HeroInstance.md)

### getUnitCaster

Returns the unit performing the cast, or nil if cast by a hero.

- returns [`Unit`](Unit.md)

### getCasterNameTextID

Returns the text ID of the caster's name.

- returns `string`

### getBattle

Returns the battle callback associated with this cast.

- returns [`Battle`](Battle.md)

### calculateRawEffectValue

Returns the raw effect value before unit-specific adjustments.

- param `basePowerMultiplier`: `integer` — Multiplier applied to the spell's base power.
- param `levelPowerMultiplier`: `integer` — Multiplier applied to the per-level power bonus.

- returns `integer`

### applySpecificSpellBonus

Applies any spell-specific bonus modifier and returns the resulting value.

- param `value`: `integer` — Base value to which spell-specific modifiers are applied. Use 0 for default

- returns `integer`

### applySpellBonus

Applies the generic spell-damage bonus and returns the resulting value.

- param `value`: `integer` — Base value to which the bonus is applied. Use 0 for default.
- param `target`: [`Unit`](Unit.md) — Unit whose own bonuses are factored into the result.

- returns `integer`

### isReceptive

True if the target is receptive (not immune) to the spell.

- param `target`: [`Unit`](Unit.md) — Unit whose receptivity is being checked.

- returns `boolean`

### ownerMatches

True if the given unit is owned by the same player as the caster.

- param `unit`: [`Unit`](Unit.md) — Unit whose ownership is being compared against the caster's.

- returns `boolean`

### getSpell

Returns the Spell being cast.

- returns [`Spell`](Spell.md)

### adjustEffectValue

Applies all per-target adjustments to the raw effect value.

- param `target`: [`Unit`](Unit.md) — Unit against which per-target adjustments are computed.

- returns `integer`

### getPluralFormTextID

Picks the appropriate plural-form variant of a text ID for the given count and language.

- param `baseTextID`: `string` — Base text ID used as the lookup base.
- param `count`: `integer` — Count for which engine needs to select plural form.

- returns `string`
