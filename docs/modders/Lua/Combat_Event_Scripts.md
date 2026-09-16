# Combat Event Scripts

Declared with `"implements" : "combatEvent"` in the [scripts](Script_Types.md) section of a mod. This script type reacts to events that happen to a unit during a battle, such as the unit waiting or being attacked. It is the way to implement effects that are not immediate - for example a spell that does something on the turn of every unit it affected.

## Attaching a script to a unit

A combat script runs on a unit that has the [COMBAT_EVENT_TRIGGER](../Bonus/Bonus_Types.md#combat_event_trigger) bonus referring to it. The bonus can be given like any other bonus - as a creature ability, from an artifact, from a secondary skill:

```json
"drainsLife" : {
    "type" : "COMBAT_EVENT_TRIGGER",
    "subtype" : "lifeDrain",
    "val" : 100
}
```

A spell grants the same bonus for its own duration through the built-in [attachCombatScript](../Entities_Format/Spell_Format.md#attach-combat-script) spell effect:

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

`eventParameters` reaches the script exactly as written, and is checked against the `schema` the script declares, so a spell attaching a script has to pass whatever that script requires. Nothing about the caster is added automatically - a script that needs to know the spell or its power has to be given it here.

## Description shown to the player

Every scripted ability shares the same `COMBAT_EVENT_TRIGGER` bonus type, so the text shown to the player comes from the script rather than from the bonus. Declare it as `description` on the script itself; `${val}` is replaced with the total value of every bonus granting this script and `${parameterName}` with the value the bonus passed in its parameters:

```json
"lifeDrain" : {
    "implements" : "combatEvent",
    "script" : "combat/lifeDrain",
    "patches" : [ ],
    "priority" : 0,
    "schema" : { "properties" : {}, "additionalProperties" : false },
    "description" : "{Life Drain}\nRestores health equal to ${val}% of damage dealt."
}
```

## Writing a script

Scripts extend `combatScript` and define a method only for the events they actually react to. An event whose method the script does not define is never handed to it:

```lua
local Base = require("combat/combatScript")
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

- every event is delivered to every script attached to the unit it happened to that defines a method for it. An event the script does not define a method for is skipped
- the parameters stored in the bonus are read-only. A script that needs to remember something between events must store it itself, for example in a bonus of its own
- an action reaches `onActionFinished` exactly once, however many blows, counterattacks or targets it was made of
- a script cannot cause further combat events, except `onDeath`. Combat events are fired by actions, and everything a script can do only changes battle state; a spell a script casts is applied rather than cast, so it fires no `onSpellHit` either. Deaths a script causes are announced after it returns, never while it is still running
- a script never starts while another one is running. Reactions run one after another to completion, whichever event they handle
- no event is withheld from a script because the engine judged it pointless. Whether a counterattack, a repeated blow or the death of the bearer is a reason to do nothing is decided by the script

## Event handlers

All handlers share the same signature and return nothing:

Signature: `function Script:on<Event>(server, battle, unit, other, payload)`

The parameters stored in the bonus initialize the script, so every property defined there is available as a field of `self` - for example `addInfo` of `{ "poison" : true }` is read as `self.poison`. The magnitude of the ability lives in the bonus value rather than in the parameters, and is read as `self.val`.

When several bonuses on the same unit grant the same script, each of them runs the script once, with its own `val` - the values are not summed into a single run. An ability whose effect is proportional to `val` therefore adds up on its own, while one that rolls a chance rolls once per bonus rather than once at the combined chance. When only the strongest source should apply instead, give every bonus granting the script the same [`stacking`](../Bonus_Format.md) group, the way core content does for fire shield.

Parameters:

- `server` - used to apply actual changes to the battle state. See [BattleServer](../Lua_Reference/BattleServer.md).
- `battle` - state of the battle this event happened in. See [Battle](../Lua_Reference/Battle.md).
- `unit` - the unit carrying the bonus, which this event happened to. See [Unit](../Lua_Reference/Unit.md).
- `other` - the unit on the opposite side of the event, such as the attacker. May be nil.
- `payload` - data about the attack or spell that caused this event. Every event is handed one, and the fields an event does not fill keep their empty value, so a handler may read the fields it cares about without checking which event fired:
  - `ranged` - whether the attack was a shot
  - `isCounter` - whether the attack is a counterattack, either a first strike or a regular retaliation
  - `attackIndex` - position of this attack among those its own side makes in this action, so `0` for the first blow and `1` for the second of a double attack. A counterattack is its own side's attack `0`
  - `targets` - one entry per unit the attack reaches. Each holds the `unit` itself, the `damage` dealt to it, how many of its creatures were `killed`, the `damageBeforeDefense` this same blow would have dealt with the target's defences ignored, and the `healthBeforeAttack` the unit had left before the hit landed. **Before** the attack only `unit` and `healthBeforeAttack` are known - no damage has been rolled yet, so the other fields are zero
  - `spell` - the spell that caused this event, for `onUnitSpellcast` and `onSpellHit`. Nil for every other event, so an ability that reacts to one particular spell tests it rather than assuming which one fired

  For `onSpellHit` every entry also holds `unitBefore` - the target as it was before the spell reached it. Compare it against the unit as it is now to tell the effect of the spell from what the unit already was. Every other event leaves it nil.

  A handler receives the whole target list, not only its own entry, so that it can see the full attack. `self:ownEntry(unit, payload)` returns the entry of the handling unit. `target.unit` is nil for a unit the event removed from the battlefield, so check it before use when walking the list directly.

Handlers:

- `onBeforeAttack` - called on the attacker before every one of its attacks, retaliations included
- `onBeforeAttacked` - called on every unit the attack is about to reach, not only its primary target, before every attack
- `onAfterAttack` - called on the attacker once its attack is resolved. The attacker may be dead by then, killed by a reaction to its own attack, so a script that must not act from a dead unit checks `unit:isAlive()` itself
- `onAfterAttacked` - called on every unit the attack hit, once that attack is resolved. Fires even when the attack killed `unit`, so that a reflecting ability can still react to a lethal blow. A script that must not react from a dead unit checks `unit:isAlive()` itself
- `onWait` - called when `unit` waits
- `onDefend` - called when `unit` defends
- `onBeforeMove` - called before `unit` starts movement. See [Moves](#moves) for what counts as one
- `onAfterMove` - called after `unit` ends movement, once for every `onBeforeMove`
- `onUnitSpellcast` - called after `unit` casts a spell
- `onSpellHit` - called on every unit a spell reached, once the cast is over. `other` is the casting unit, nil when a hero cast it. See [Spell hits](#spell-hits) for which casts are reported
- `onDeath` - called on a unit that died, once the action that killed it is over. `other` is the unit that killed it, nil for a death caused by a spell, a moat or a script. See [Deaths](#deaths)
- `onActionFinished` - called once the battle action is fully over, on every unit it reached. `other` is the unit that acted, nil when a hero cast a spell. See [The end of an action](#the-end-of-an-action)
- `onBattleSetup` - called once for every unit as the battle is laid out, before tactics and before anything else happens to it. `other` is nil
- `onBattleStart` - called once for every unit present when the battle starts, after tactics are over and before any opening spell is cast. `other` is nil
- `onRoundStart` - called for every alive unit at the start of each round after the first. The first round is covered by `onBattleStart`. `other` is nil

![Order in which combat events fire](Combat_Event_Flow.svg)

The attack events always fire in this order, once per attack:

```text
onBeforeAttack   (attacker)  \  one group, ordered by priority
onBeforeAttacked (each unit about to be hit)  /
      ... damage is rolled and applied, the combat log is written ...
onAfterAttack    (attacker)  \  one group, ordered by priority
onAfterAttacked  (each unit that was hit)  /
```

The attacker and the units it hits react as one ordered group, so `priority` alone decides whether a script runs before or after another; which side of the attack it belongs to does not matter. This is why life drain (priority 0) heals before a fire shield (priority 50) burns the attacker down.

None of them is withheld: a counterattack, the second blow of a double attack and an attack whose bearer dies mid-resolution all deliver the full sequence. Whether to act is decided by the script, since the correct answer differs per ability - a fire shield must burn its killer while dying, a death stare must not kill anyone once its bearer is gone.

### Moves

A move is any movement of the unit itself: a move action, the walk of a walk-and-attack or of an adjacent spellcaster, and the step back of a unit with `RETURN_AFTER_STRIKE`. The step back is a separate move, so such an attack reports two moves. An action that reaches its target from the hex the unit already stands on fires neither move event.

`onAfterMove` fires once for every `onBeforeMove`, including a move the game then refuses because a reaction left the destination out of reach. For a walk-and-attack both events fire before the blows of that attack are counted, so an extra attack granted there is used by that same attack.

### Spell hits

Only a deliberate cast is a spell hit: a hero spell, a creature spellcaster, an enchanter. A moat, a spell-like attack, a spell triggered by an obstacle and a spell applied by a script are not casts and fire no event.

### Deaths

Deaths are announced once the action that caused them is over, not at the moment a unit falls, and all deaths of one action are announced together. Reactions may kill further units, which are announced next, until no deaths are left - so one death can trigger another. A clone is announced like any other unit. `payload.targets` holds one entry per death of the batch; its `killed` is the number of creatures the lethal hit took, which is the size of the stack as it died.

### The end of an action

`onActionFinished` reaches every unit the action touched: the unit that acted, every unit it struck, every unit a spell of that action reached. An attack action is over only once its counterattack and its repeated blows are done, so this is where a script applies what it accumulated over the whole action instead of blow by blow. To tell its own action from another unit's, a script compares `other` against `unit`.

## Built-in scripts

Every combat event script declares a `priority` in its `scripts` entry. Scripts reacting to the same event run from lowest priority to highest, `0` being the usual value. It is required and has no default because bonus order is otherwise alphabetical by ability name, which would let a mod change execution order by renaming an ability.

Priority also decides the order in which deaths are handled. A script that undoes a death by resurrecting the stack declares a priority below 100, so that it runs before scripts that react to a death; a script that reacts to a death declares 100 or above.

### ballistaDamage

Scales damage of a war machine by the attack of the hero owning it. Only the hero's own skill and its equipped artifacts count; attack granted by an army or by a spell does not. The damage is calculated once, when the battle is set up, and granted as a `CREATURE_DAMAGE` bonus, so that every window, tooltip and damage roll reads the same number.

Override `getDamageRange(unit, minDamage, maxDamage)` in a patch to change the formula. It returns the resulting damage of the machine, and the script grants the difference from the damage of its creature.

### arrowTowerDamage

Calculates damage of an arrow tower from the buildings of the town it defends, and grants it the same way and at the same moment as the script above. Outside a siege there is no town to read, and the tower keeps the damage of its creature.

Parameters:

- `keepBase` - damage of the keep in a town with nothing built
- `towerBase` - damage of the two lesser towers in a town with nothing built
- `perBuilding` - damage each building adds to the keep; the lesser towers get half of it

The scripts below exist only so that content declaring the bonus they replaced keeps working. They reproduce the H3 and WoG behaviour they were converted from, quirks included, and will not receive options beyond what that behaviour needs. A mod that needs an ability of this kind should ship its own script instead of configuring these.

### rebirth

Resurrects its bearer once per battle, with a share of the size the stack started the battle with, not of what was left of it. The share rarely divides evenly, so the remainder is rolled for, one chance per creature it fell short of. A clone is not resurrected, and a resurrected stack cannot retaliate until its next turn.

Priority 0, so that the stack is resurrected before scripts that react to a death.

Parameters:

- `val` - share of the starting size of the stack that is resurrected, in percent
- `guaranteed` - if true, at least one creature is always resurrected, whatever the share

### lifeDrain

Restores part of the damage its bearer dealt back to it as health, resurrecting fallen creatures of the stack. Only damage dealt to living targets counts.

Priority 0, so that the drain heals before scripts that react to the attack can kill the attacker.

Parameters:

- `val` - share of the dealt damage restored to the attacker, in percent

### fireShield

Burns whoever strikes its bearer in melee for a share of the damage that strike could have dealt. An attacker immune to fire takes nothing, and neither does one that an area attack reached without closing with the bearer.

Priority 50.

Parameters:

- `val` - share of the reflected damage, in percent

### deathStare

Kills creatures of the attacked stack outright, each creature of the bearer's stack rolling its own chance. At most the share of the stack that could have rolled it dies.

Priority 100, so that it runs after scripts that may have killed the bearer, which cancels the gaze.

Parameters:

- `val` - chance for each creature to kill one, in percent
- `situation` - when the ability applies: `"melee"`, `"ranged"`, `"rangedDistancePenalty"`, `"rangedWallPenalty"` or `"rangedDistanceAndWallPenalty"`
- `spell` - spell cast to kill them, which decides the animation, the immunities and the wording of the combat log. Defaults to death stare

The number of kills is decided by `killsIn`, which returns nil when the ability does not apply to the attack. A mod adds a situation of its own by overriding that method in a patch; `combat/deathStareCommander`, the patch core stacks over this script, is the worked example:

```lua
function Script:killsIn(server, battle, unit, other, payload)
    if self.situation ~= "commander" then
        return Base.killsIn(self, server, battle, unit, other, payload)
    end

    return <however many this ability kills>
end
```

`"commander"` is DEPRECATED and comes from that patch, not from the script. It exists so that the commander skill converted from the `DEATH_STARE` bonus keeps working, and `val` has a different meaning under it: kills before the level ratio of the two stacks is applied. Write a patch instead of expecting more situations to be added here.

### enchanted

Keeps a spell permanently applied to its bearer, or to its whole side, by re-applying it at the start of every round.

Parameters:

- `spell` - spell whose effects are applied
- `level` - mastery level the effects are applied at
- `massive` - true to affect every allied unit instead of only the bearer
- `duration` - how many turns the effects last. Defaults to 50, long enough for the effect to accumulate instead of expiring between rounds

### summonGuardians

DEPRECATED, transition only - see the note at the start of this section.

Surrounds its bearer with summoned guardians when the battle starts. Guardians are placed as in H3, including the special cases for units starting against their own edge of the battlefield.

Parameters:

- `creature` - creature to summon as guardian
- `val` - size of each guardian stack, in percent of the guarded stack

### transmutation

DEPRECATED, transition only - see the note at the start of this section.

Replaces the attacked stack with a stack of another creature, as the WoG werewolf ability does. A unit with [TRANSMUTATION_IMMUNITY](../Bonus/Bonus_Types.md#transmutation_immunity) is not affected, and neither is a non-living one.

Priority 300.

Parameters:

- `val` - percentage chance to trigger on each attack
- `creature` - creature the victim turns into. Defaults to the attacker's own creature
- `transmuteBy` - `"health"` keeps the total health of the victim, `"count"` keeps its creature count

### soulSteal

DEPRECATED, transition only - see the note at the start of this section.

Raises its bearer's stack for every enemy creature it killed, beyond the stack's original size. Only kills among living targets count.

Parameters:

- `val` - creatures gained for each killed enemy creature
- `permanent` - true to keep the gained creatures after the battle

### destruction

DEPRECATED, transition only - see the note at the start of this section.

Kills creatures of the attacked stack outright, on top of the damage the attack itself dealt.

Priority 400.

Parameters:

- `val` - percentage chance to trigger on each attack
- `killBy` - `"percentage"` kills a share of the victim's stack, `"count"` kills a fixed number
- `amount` - the share, or the number of creatures, depending on `killBy`
