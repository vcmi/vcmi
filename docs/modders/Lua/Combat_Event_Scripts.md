# Combat Event Scripts

Declared with `"implements" : "combatEvent"` in the [scripts](Script_Types.md) section of a mod. This script type reacts to events that happen to a unit during a battle, such as the unit waiting or being attacked. It is the way to implement effects that are not immediate - for example a spell that does something on the turn of every unit it affected.

A combat script runs on a unit that has the [COMBAT_EVENT_TRIGGER](../Bonus/Bonus_Types.md#combat_event_trigger) bonus referring to it. The bonus can be given like any other bonus, or granted by a spell via the built-in `attachCombatScript` spell effect:

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

`eventParameters` reaches the script exactly as written, and is checked against the `schema` the
script declares, so a spell attaching a script has to pass whatever that script requires. Nothing
about the caster is added automatically - a script that needs to know the spell or its power has to
be given it here.

Every scripted ability shares the same `COMBAT_EVENT_TRIGGER` bonus type, so the text shown to the player comes from the script rather than from the bonus. Declare it as `description` on the script itself; `${val}` is replaced with the accumulated value of the bonus and `${parameterName}` with the value the bonus passed in its parameters:

```json
"lifeDrain" : {
    "type" : "lua",
    "implements" : "combatEvent",
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

## Assumptions and guarantees

VCMI guarantees the following:

- every event is delivered to every script attached to the unit it happened to. Events that a script does not implement resolve to the no-op inherited from `combatScript`
- the parameters stored in the bonus are read-only. A script that needs to remember something between events must store it itself, for example in a bonus of its own
- a script cannot cause further combat events. Everything it can do only changes battle state, and combat events are fired by unit actions

### Available functions

All functions share the same signature and return nothing:

Signature: `function Script:on<Event>(server, battle, unit, other)`

The parameters stored in the bonus initialize the script, so every property defined there is available as a field of `self` - for example `addInfo` of `{ "poison" : true }` is read as `self.poison`. The magnitude of the ability lives in the bonus value rather than in the parameters, and is read as `self.val`; it is the only field that accumulates when several sources grant the same ability.

Parameters:

- `server` - used to apply actual changes to the battle state. See [Server](API_Reference.md#server).
- `battle` - state of the battle this event happened in. See [Battle](API_Reference.md#battle).
- `unit` - the unit carrying the bonus, which this event happened to. See [Unit](API_Reference.md#unit).
- `other` - the unit on the opposite side of the event, such as the attacker. May be nil.
- `payload` - data specific to this event, empty for events that carry none. `onAttackResolved` and `onAfterAttacked` fill `payload.targets` with one entry per unit the attack hit, and set `payload.ranged` for a shot. Each entry holds the `unit` itself, the `damage` dealt to it, how many of its creatures were `killed`, the `damageBeforeDefense` the attack could have dealt with the target's defence ignored, and the `healthBeforeAttack` the unit had left before the hit landed. `onAfterAttacked` receives the whole list rather than only its own entry, so a script can see the full attack; it finds itself by comparing `target.unit` against `unit`.

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

### Built-in scripts

A script may declare a `priority` in its `scripts` entry. Scripts reacting to the same event run from lowest priority to highest; scripts that declare none keep the default of 0. Bonus order is otherwise alphabetical by ability name, which would let a mod decide what runs first by renaming an ability.

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
- `fireShield` - burns whoever strikes its bearer in melee for a share of the damage that strike could have dealt. An attacker immune to fire takes nothing. Parameters:
    - `val` - share of the reflected damage, in percent
- `deathStare` - kills creatures of the attacked stack outright, each creature of the bearer's stack rolling its own chance. At most the share of the stack that could have rolled it dies. Parameters:
    - `val` - chance for each creature to kill one, in percent. For `"commander"` it is instead a flat number of kills, scaled by the level ratio of the two stacks
    - `situation` - when the ability applies: `"melee"`, `"ranged"`, `"rangedDistancePenalty"`, `"rangedWallPenalty"`, `"rangedDistanceAndWallPenalty"` or `"commander"`
    - `spell` - spell cast to kill them, which decides the animation, the immunities and the wording of the combat log. Defaults to death stare
- `destruction` - kills creatures of the attacked stack outright, on top of the damage the attack itself dealt. Parameters:
    - `val` - percentage chance to trigger on each attack
    - `killBy` - `"percentage"` kills a share of the victim's stack, `"count"` kills a fixed number
    - `amount` - the share, or the number of creatures, depending on `killBy`
- `enchanted` - keeps a spell permanently applied by re-applying it every round. Parameters:
    - `spell` - spell whose effects are applied
    - `level` - mastery level the effects are applied at
    - `massive` - true to affect every allied unit instead of only the bearer
    - `duration` - how many turns the effects last. Defaults to 50, long enough for the effect to accumulate rather than expire between rounds
