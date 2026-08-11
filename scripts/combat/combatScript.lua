local Script = {}
Script.__index = Script
Script.type = "combatScript"

--- Base class for scripts attached to units via the COMBAT_EVENT_TRIGGER bonus.
--- A script is called on every combat event that happens to its bearer, so it only needs to
--- override the events it actually reacts to - the rest fall through to the no-ops below.
---
--- In every handler `unit` is the bearer of the bonus and `other` is the unit on the opposite
--- side of the event (the attacker, the victim, ...), which may be nil.
--- Parameters stored in the bonus are available as fields on `self` and are read-only;
--- a script that needs to remember something across events must store it itself,
--- for example in a bonus of its own.
---
--- No event is withheld: the attack handlers below fire for a counterattack, for every blow of a
--- multiple attack, and even when the bearer died while the attack was resolving. Whether that is
--- a reason to do nothing is the script's own call - a reflecting ability answers a lethal blow
--- while dying, while an ability that strikes back must first check `unit:isAlive()`.

--- Called before `unit` attacks `other`. `payload.targets` names everyone the attack is about to
--- reach, with only their remaining health known - no damage has been rolled yet.
function Script:onBeforeAttack(server, battle, unit, other, payload)
end

--- Called after `unit` attacked. Whether this runs before or after the units it hit react is
--- decided by the script's `priority` alone - one shared order covers the attacker and its victims.
--- `unit` may be dead by then, killed by a reaction to its own attack.
function Script:onAfterAttack(server, battle, unit, other, payload)
end

--- Called before `unit` is attacked by `other`, on every unit the attack is about to reach.
function Script:onBeforeAttacked(server, battle, unit, other, payload)
end

--- Called after `unit` was attacked by `other`, even if the attack killed `unit`.
function Script:onAfterAttacked(server, battle, unit, other, payload)
end

--- Called when `unit` waits.
function Script:onWait(server, battle, unit, other)
end

--- Called when `unit` defends.
function Script:onDefend(server, battle, unit, other)
end

--- Called before `unit` moves.
function Script:onBeforeMove(server, battle, unit, other)
end

--- Called after `unit` moved.
function Script:onAfterMove(server, battle, unit, other)
end

--- Called when `unit` casts a spell.
function Script:onUnitSpellcast(server, battle, unit, other)
end

--- Called once for every unit present when the battle starts, after tactics are over.
function Script:onBattleStart(server, battle, unit, other)
end

--- Called for every alive unit at the start of each round after the first.
--- The first round is covered by `onBattleStart`.
function Script:onRoundStart(server, battle, unit, other)
end

return Script
