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
        "eventValue" : 10,
        "eventParameters" : { "poison" : true }
    }
}
```

Every scripted ability shares the same `COMBAT_EVENT_TRIGGER` bonus type, so the text shown to the player comes from the script rather than from the bonus. Declare it as `description` on the script itself; `${val}` is replaced with the accumulated value of the bonus and `${parameterName}` with the value the bonus passed in its parameters:

```json
"lifeDrain" : {
    "type" : "lua",
    "script" : "lifeDrain",
    "description" : "{Life Drain}\nRestores health equal to ${val}% of damage dealt."
}
```

A script without a `description` shows nothing, which is the way to keep an ability out of the creature window.

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
- the parameters stored in the bonus are read-only. A script that needs to remember something between events must store it itself, for example in a bonus of its own
- a script cannot cause further combat events. Everything it can do only changes battle state, and combat events are fired by unit actions

#### Available functions

All functions share the same signature and return nothing:

Signature: `function Script:on<Event>(server, battle, unit, other)`

The parameters stored in the bonus initialize the script, so every property defined there is available as a field of `self` - for example `addInfo` of `{ "poison" : true }` is read as `self.poison`. The magnitude of the ability lives in the bonus value rather than in the parameters, and is read as `self.val`; it is the only field that accumulates when several sources grant the same ability.

Parameters:

- `server` - used to apply actual changes to the battle state. See [Server](Api_Reference.md#server).
- `battle` - state of the battle this event happened in. See [Battle](Api_Reference.md#battle).
- `unit` - the unit carrying the bonus, which this event happened to. See [Unit](Api_Reference.md#unit).
- `other` - the unit on the opposite side of the event, such as the attacker. May be nil.
- `payload` - data specific to this event, empty for events that carry none. `onAttackResolved` and `onAfterAttacked` fill `payload.targets` with one entry per unit the attack hit, each holding the `unit` itself, the `damage` dealt to it and how many of its creatures were `killed`. `onAfterAttacked` receives the whole list rather than only its own entry, so a script can see the full attack; it finds itself by comparing `target.unit` against `unit`.

Functions:

- `onBeforeAttack` - called before `unit` attacks `other`
- `onAfterAttack` - called after `unit` attacked `other` and every ability triggered by that attack has resolved. Fires even if the attack killed `other`, so a script may safely replace or remove it
- `onBeforeAttacked` - called before `unit` is attacked by `other`
- `onAfterAttacked` - called on `unit` once the attack that hit it is resolved, before the attacker's follow-up abilities. Fires for every unit the attack hit, not only its primary target, and fires even when the attack killed `unit`, so a script that should not react to a dead unit has to check for itself. Receives a `payload`, see below
- `onWait` - called when `unit` waits
- `onDefend` - called when `unit` defends
- `onBeforeMove` - called before `unit` starts movement
- `onAfterMove` - called after `unit` ends movement
- `onUnitSpellcast` - called after `unit` casts a spell
- `onBattleStart` - called once for every unit present when the battle starts, after tactics are over. `other` is nil
- `onRoundStart` - called for every alive unit at the start of each round after the first. The first round is covered by `onBattleStart`. `other` is nil
- `onAttackResolved` - called on the attacker once its attack is fully resolved, before any reaction of the units it hit. Receives a `payload`, see below

#### Built-in scripts

- `transmutation` - replaces the attacked stack with a stack of another creature, as the WoG werewolf ability does. Parameters:
    - `val` - percentage chance to trigger on each attack
    - `creature` - creature the victim turns into. Defaults to the attacker's own creature
    - `transmuteBy` - `"health"` keeps the total health of the victim, `"count"` keeps its creature count
- `summonGuardians` - surrounds its bearer with summoned guardians when the battle starts. Parameters:
    - `creature` - creature to summon as guardian
    - `val` - size of each guardian stack, in percent of the guarded stack
- `lifeDrain` - restores part of the damage its bearer dealt back to it as health, resurrecting fallen creatures of the stack. Only damage dealt to living targets counts. Parameters:
    - `val` - share of the dealt damage restored to the attacker
- `soulSteal` - raises its bearer's stack for every enemy creature it killed, beyond the stack's original size. Only kills among living targets count. Parameters:
    - `val` - creatures gained for each killed enemy creature
    - `permanent` - true to keep the gained creatures after the battle
- `enchanted` - keeps a spell permanently applied by re-applying it every round. Parameters:
    - `spell` - spell whose effects are applied
    - `level` - mastery level the effects are applied at
    - `massive` - true to affect every allied unit instead of only the bearer
    - `duration` - how many turns the effects last. Defaults to 50, long enough for the effect to accumulate rather than expire between rounds
