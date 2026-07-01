# BonusDescriptor

Configuration for a Bonus that scripts hand to ServerCallback `addUnitBonus` or `addBattleBonus`. Fields mirror the JSON bonus definition. See bonus docs for more details.

### val

Numeric magnitude of the bonus (interpreted per `valueType`).

- type: `integer`

### turns

Remaining duration in turns; relevant for the N_TURNS / N_DAYS durations.

- type: `integer`

### hidden

If true, the bonus is not shown in the unit's bonus list UI.

- type: `boolean`

### stacking

Stacking key — bonuses sharing it overwrite rather than accumulate.

- type: `string`

### description

Optional human-readable description (used by tooltips when shown). Overrides generic description for this bonus type.

- type: `string`

### icon

Optional icon name shown next to the bonus in the UI. Overrides generic icon for this bonus type.

- type: `string`

### type

Bonus type name (e.g. "PRIMARY_SKILL", "FIRE_IMMUNITY"). See Bonus types documentation.

- type: `any`

### subtype

Sub-selector that narrows the type (e.g. specific creature, school, primary stat).

- type: `any`

### valueType

How `val` is combined with other bonuses: additive, base, percent-of-..., independent min/max.

- type: `any`

### effectRange

Spatial scope the bonus applies in (e.g. ranged-only or melee-only).

- type: `any`

### duration

When the bonus expires — permanent, one-battle, n-turns, until-attacked, etc.

- type: `any`

### sourceType

Origin class (artifact, spell effect, secondary skill, …) — drives source-based dispels.

- type: `any`

### sourceID

Identifier of the specific source within its sourceType.

- type: `any`

### targetSourceType

Source type the bonus is restricted to act upon (used by hero specialty bonuses).

- type: `any`

### addInfo

Optional auxiliary payload — meaning depends on the bonus type.

- type: `any`

### limiters

JSON-defined limiter chain that definea whether the bonus applies to a given bearer.

- type: `any`

### propagator

Rule for propagating the bonus upwards for area effect (army-wide, player-wide, …).

- type: `any`

### updater

Rules for recalculation of bonus parameters (e.g. scales with stack count).

- type: `any`

### propagationUpdater

Updater applied to bonuses produced by this one's propagator.

- type: `any`
