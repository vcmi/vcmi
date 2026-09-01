# Spell Effect Scripts

Declared with `"implements" : "spellEffect"` in the [scripts](Script_Types.md) section of a mod. This script type is used to implement spell effects, similar to built-in `core:summon`, `core:damage`, `core:heal`, and similar.

Generally, when spell is being cast game will make following calls:

- `applicableGeneral` when hero attepts to pick spell from a spellbook
- `transformTarget` and `applicableTarget` when hero hovers spell over potential target
- `apply` when player attempts to finish casting the spell

## Assumptions and guarantees

WARNING: Make sure to read this section before writing the script! Not following them may result in hard to understand bugs!

VCMI guarantees the following:

- if `applicableGeneral` returns false, no other methods will be called for a script
- if `applicableTarget` returns false, `apply` will not be called with such target

VCMI does NOT guarantees:

- any specific order or number of calls other than those specified in this section. Game may call `applicableTarget` multiple times, or even call `apply` without spell actually having an effect when AI estimates spells
- global state of the script  is not guaranteed to remain the same between calls

### Available functions

All spell parameters provided in the spell effect json config initialize the script, so every property defined there is available as a field of `self` - for example a config of `{ "cumulative" : true }` is read as `self.cumulative`.

### applicableGeneral

Signature: `function Script:applicableGeneral(mechanics, problem)`

This function should return true if spell effect has at least one valid target on which it can be cast, or false if none of entities (such as units or hexes) can be used as target for the spell.

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](../Lua_Reference/SpellMechanics.md).
- `problem` - storage for any "problems" with casting the spell. If spell can't be casted, reason for the failure must be added to the problem. See [SpellProblem](../Lua_Reference/SpellProblem.md).

Return value: boolean

### transformTarget

Signature: `function Script:transformTarget(mechanics, aimPoint, spellTarget)`

This function should examine `aimPoint` and `spellTarget` to generate list of targets that are affected by the spell

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](../Lua_Reference/SpellMechanics.md).
- `aimPoint` - TODO. A `Destination[]`.
- `spellTarget` - TODO. A `Destination[]`.

Return value: `Destination[]`

### applicableTarget

Signature: `function Script:applicableTarget(mechanics, problem, target)`

This function should return true if spell can be cast on a specified target(s).

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](../Lua_Reference/SpellMechanics.md).
- `problem` - storage for any "problems" with casting the spell. If spell can't be casted, reason for the failure must be added to the problem. See [SpellProblem](../Lua_Reference/SpellProblem.md).
- `target` - Target (such as unit or hex) on which this spell is being cast, after convertion by `transformTarget`. A `Destination[]`.

Return value: boolean

### filterTarget

Signature: `function Script:filterTarget(mechanics, target)`

This function should remove from `target` any destinations that should not receive the effect.

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](../Lua_Reference/SpellMechanics.md).
- `target` - Target (such as unit or hex) on which this spell is being cast, after convertion by `transformTarget`. A `Destination[]`.

Return value: `Destination[]`

### apply

Signature: `function Script:apply(mechanics, server, target)`

This function performs actual cast of the spell and applies all effects caused by the spell to game via `server` parameter. It is guaranteed that `target` has been transformed via `transformTarget` and verified to be applicable via calls to `applicableGeneral` and `applicableTarget`

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](../Lua_Reference/SpellMechanics.md).
- `server` - This parameter can be used to apply actual changes in a battle state [BattleServer](../Lua_Reference/BattleServer.md).
- `target` - Target (such as unit or hex) on which this spell is being cast, after convertion by `transformTarget`. A `Destination[]`.

Return value: nothing

### getHealthChange

Signature: `function Script:getHealthChange(mechanics, spellTarget)`

This function should return the health and unit count change the spell is predicted to cause, used by AI and by hover text. Effects that neither heal nor damage keep the neutral value of `{ hpDelta = 0, unitsDelta = 0 }`.

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](../Lua_Reference/SpellMechanics.md).
- `spellTarget` - Target (such as unit or hex) on which this spell is being cast. A `Destination[]`.

Return value: table with `hpDelta` and `unitsDelta` fields

### adjustAffectedHexes

Signature: `function Script:adjustAffectedHexes(mechanics, hexes, spellTarget)`

This function should add to `hexes` any hexes that the spell affects, so that they can be highlighted for the player.

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](../Lua_Reference/SpellMechanics.md).
- `hexes` - hexes collected so far. See [BattleHexArray](../Lua_Reference/BattleHexArray.md).
- `spellTarget` - Target (such as unit or hex) on which this spell is being cast. A `Destination[]`.

Return value: [BattleHexArray](../Lua_Reference/BattleHexArray.md)

### adjustTargetTypes

Signature: `function Script:adjustTargetTypes(mechanics, types)`

This function should alter the list of target types that this effect accepts.

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](../Lua_Reference/SpellMechanics.md).
- `types` - target types collected so far

Return value: list of target types
