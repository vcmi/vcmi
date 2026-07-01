# Script Types

### Spell Effect

This script type is used to implement spell effects, similar to built-in `core:summon`, `core:damage`, `core:heal`, and similar.

Generally, when spell is being cast game will make following calls:

- `applicable` when hero attepts to pick spell from a spellbook
- `transformTarget` and `applicableTarget` when hero hovers spell over potential target
- `apply` when player attempts to finish casting the spell

### Assumptions and guarantees

WARNING: Make sure to read this section before writing the script! Not following them may result in hard to understand bugs!

VCMI guarantees the following:

- if `applicable` returns false, no other methods will be called for a script
- if `applicableTarget` returns false, `apply` will not be called with such target

VCMI does NOT guarantees:

- any specific order or number of calls other than those specified in this section. Game may call `applicableTarget` multiple times, or even call `apply` without spell actually having an effect when AI estimates spells
- global state of the script  is not guaranteed to remain the same between calls

#### Available functions

#### applicable

Signature: `applicable = function(parameters, mechanics, problem)`

This function should return true if spell effect has at least one valid target on which it can be cast, or false if none of entities (such as units or hexes) can be used as target for the spell.

Parameters:

- `parameters` - contains all spell parameters provided in spell effect json config
- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](Api_Reference.md#spellmechanics).
- `problem` - storage for any "problems" with casting the spell. If spell can't be casted, reason for the failure must be added to the problem. See [SpellProblem](Api_Reference.md#spellproblem).

Return value: boolean

#### transformTarget

Signature: `transformTarget = function(parameters, mechanics, aimPoint, spellTarget)`

This function should examine `aimPoint` and `spellTarget` to generate list of targets that are affected by the spell

Parameters:

- `parameters` - contains all spell parameters provided in spell effect json config
- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](Api_Reference.md#spellmechanics).
- `aimPoint` - TODO. See [SpellTarget](Api_Reference.md#spelltarget).
- `spellTarget` - TODO. See [SpellTarget](Api_Reference.md#spelltarget).

Return value: [SpellTarget](Api_Reference.md#spelltarget)

#### applicableTarget

Signature: `applicableTarget = function(parameters, mechanics, problem, target)`

This function should return true if spell can be cast on a specified target(s).

Parameters:

- `parameters` - contains all spell parameters provided in spell effect json config
- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](Api_Reference.md#spellmechanics).
- `problem` - storage for any "problems" with casting the spell. If spell can't be casted, reason for the failure must be added to the problem. See [SpellProblem](Api_Reference.md#spellproblem).
- `target` - Target (such as unit or hex) on which this spell is being cast, after convertion by `transformTarget` See [SpellTarget](Api_Reference.md#spelltarget).

Return value: boolean

#### apply

Signature: `apply = function(parameters, mechanics, server, target)`

This function performs actual cast of the spell and applies all effects caused by the spell to game via `server` parameter. It is guaranteed that `target` has been transformed via `transformTarget` and verified to be applicable via calls to `applicable` and `applicableTarget`

Parameters:

- `parameters` - contains all spell parameters provided in spell effect json config
- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](Api_Reference.md#spellmechanics).
- `server` - This parameter can be used to apply actual changes in a battle state [Server](Api_Reference.md#server).
- `target` - Target (such as unit or hex) on which this spell is being cast, after convertion by `transformTarget` See [SpellTarget](Api_Reference.md#spelltarget).

Return value: nothing
