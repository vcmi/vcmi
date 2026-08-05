# Script Types

### Spell Effect

This script type is used to implement spell effects, similar to built-in `core:summon`, `core:damage`, `core:heal`, and similar.

Generally, when spell is being cast game will make following calls:

- `applicableGeneral` when hero attepts to pick spell from a spellbook
- `transformTarget` and `applicableTarget` when hero hovers spell over potential target
- `apply` when player attempts to finish casting the spell

### Assumptions and guarantees

WARNING: Make sure to read this section before writing the script! Not following them may result in hard to understand bugs!

VCMI guarantees the following:

- if `applicableGeneral` returns false, no other methods will be called for a script
- if `applicableTarget` returns false, `apply` will not be called with such target

VCMI does NOT guarantees:

- any specific order or number of calls other than those specified in this section. Game may call `applicableTarget` multiple times, or even call `apply` without spell actually having an effect when AI estimates spells
- global state of the script  is not guaranteed to remain the same between calls

#### Available functions

All spell parameters provided in the spell effect json config initialize the script, so every property defined there is available as a field of `self` - for example a config of `{ "cumulative" : true }` is read as `self.cumulative`.

#### applicableGeneral

Signature: `function Script:applicableGeneral(mechanics, problem)`

This function should return true if spell effect has at least one valid target on which it can be cast, or false if none of entities (such as units or hexes) can be used as target for the spell.

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](Api_Reference.md#spellmechanics).
- `problem` - storage for any "problems" with casting the spell. If spell can't be casted, reason for the failure must be added to the problem. See [SpellProblem](Api_Reference.md#spellproblem).

Return value: boolean

#### transformTarget

Signature: `function Script:transformTarget(mechanics, aimPoint, spellTarget)`

This function should examine `aimPoint` and `spellTarget` to generate list of targets that are affected by the spell

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](Api_Reference.md#spellmechanics).
- `aimPoint` - TODO. See [SpellTarget](Api_Reference.md#spelltarget).
- `spellTarget` - TODO. See [SpellTarget](Api_Reference.md#spelltarget).

Return value: [SpellTarget](Api_Reference.md#spelltarget)

#### applicableTarget

Signature: `function Script:applicableTarget(mechanics, problem, target)`

This function should return true if spell can be cast on a specified target(s).

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](Api_Reference.md#spellmechanics).
- `problem` - storage for any "problems" with casting the spell. If spell can't be casted, reason for the failure must be added to the problem. See [SpellProblem](Api_Reference.md#spellproblem).
- `target` - Target (such as unit or hex) on which this spell is being cast, after convertion by `transformTarget` See [SpellTarget](Api_Reference.md#spelltarget).

Return value: boolean

#### filterTarget

Signature: `function Script:filterTarget(mechanics, target)`

This function should remove from `target` any destinations that should not receive the effect.

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](Api_Reference.md#spellmechanics).
- `target` - Target (such as unit or hex) on which this spell is being cast, after convertion by `transformTarget` See [SpellTarget](Api_Reference.md#spelltarget).

Return value: [SpellTarget](Api_Reference.md#spelltarget)

#### apply

Signature: `function Script:apply(mechanics, server, target)`

This function performs actual cast of the spell and applies all effects caused by the spell to game via `server` parameter. It is guaranteed that `target` has been transformed via `transformTarget` and verified to be applicable via calls to `applicableGeneral` and `applicableTarget`

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](Api_Reference.md#spellmechanics).
- `server` - This parameter can be used to apply actual changes in a battle state [Server](Api_Reference.md#server).
- `target` - Target (such as unit or hex) on which this spell is being cast, after convertion by `transformTarget` See [SpellTarget](Api_Reference.md#spelltarget).

Return value: nothing

#### getHealthChange

Signature: `function Script:getHealthChange(mechanics, spellTarget)`

This function should return the health and unit count change the spell is predicted to cause, used by AI and by hover text. Effects that neither heal nor damage keep the neutral value of `{ hpDelta = 0, unitsDelta = 0 }`.

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](Api_Reference.md#spellmechanics).
- `spellTarget` - Target (such as unit or hex) on which this spell is being cast. See [SpellTarget](Api_Reference.md#spelltarget).

Return value: table with `hpDelta` and `unitsDelta` fields

#### adjustAffectedHexes

Signature: `function Script:adjustAffectedHexes(mechanics, hexes, spellTarget)`

This function should add to `hexes` any hexes that the spell affects, so that they can be highlighted for the player.

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](Api_Reference.md#spellmechanics).
- `hexes` - hexes collected so far. See [BattleHexArray](Api_Reference.md#battlehexarray).
- `spellTarget` - Target (such as unit or hex) on which this spell is being cast. See [SpellTarget](Api_Reference.md#spelltarget).

Return value: [BattleHexArray](Api_Reference.md#battlehexarray)

#### adjustTargetTypes

Signature: `function Script:adjustTargetTypes(mechanics, types)`

This function should alter the list of target types that this effect accepts.

Parameters:

- `mechanics` - contains settings at which spell is being cast, such as state of hero or creature that acts as caster of the spell. See [SpellMechanics](Api_Reference.md#spellmechanics).
- `types` - target types collected so far

Return value: list of target types

### Combat Script

This script type reacts to events that happen to a unit during a battle, such as the unit waiting or being attacked. It is the way to implement effects that are not immediate - for example a spell that does something on the turn of every unit it affected.

A combat script is registered in the `combatScripts` section of a mod and runs on a unit that has the [COMBAT_EVENT_TRIGGER](../Bonus/Bonus_Types.md#combat_event_trigger) bonus referring to it. The bonus can be given like any other bonus, or granted by a spell via the built-in `attachCombatScript` spell effect:

```json
"effects" : {
    "attachSpikes" : {
        "type" : "attachCombatScript",
        "eventScript" : "spikes",
        "eventParameters" : { "damage" : 10 }
    }
}
```

Scripts extend `combatScript`, which implements every event as a no-op, so a script only defines the events it actually reacts to:

```lua
local Base = require("combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:onAfterAttacked(server, battle, unit, other)
    if other and other:isAlive() then
        server:damageUnit(battle, other, self.damage)
    end
end

return Script
```

### Assumptions and guarantees

VCMI guarantees the following:

- every event is delivered to every script attached to the unit it happened to. Events that a script does not implement resolve to the no-op inherited from `combatScript`
- `eventParameters` stored in the bonus are read-only. A script that needs to remember something between events must store it itself, for example in a bonus of its own
- nesting is limited: changes made by a script may cause further combat events, but past a small depth further triggers are skipped

#### Available functions

All functions share the same signature and return nothing:

Signature: `function Script:on<Event>(server, battle, unit, other)`

The `eventParameters` stored in the bonus initialize the script, so every property defined there is available as a field of `self` - for example `eventParameters` of `{ "damage" : 10 }` is read as `self.damage`.

Parameters:

- `server` - used to apply actual changes to the battle state. See [Server](Api_Reference.md#server).
- `battle` - state of the battle this event happened in. See [Battle](Api_Reference.md#battle).
- `unit` - the unit carrying the bonus, which this event happened to. See [Unit](Api_Reference.md#unit).
- `other` - the unit on the opposite side of the event, such as the attacker. May be nil.

Functions:

- `onBeforeAttack` - called before `unit` attacks `other`
- `onAfterAttack` - called after `unit` attacked `other`
- `onBeforeAttacked` - called before `unit` is attacked by `other`
- `onAfterAttacked` - called after `unit` was attacked by `other`
- `onWait` - called when `unit` waits
- `onDefend` - called when `unit` defends
- `onBeforeMove` - called before `unit` starts movement
- `onAfterMove` - called after `unit` ends movement
- `onUnitSpellcast` - called after `unit` casts a spell
