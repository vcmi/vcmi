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

--- Called before `unit` attacks `other`.
function Script:onBeforeAttack(server, battle, unit, other)
end

--- Called after `unit` attacked `other`.
function Script:onAfterAttack(server, battle, unit, other)
end

--- Called before `unit` is attacked by `other`.
function Script:onBeforeAttacked(server, battle, unit, other)
end

--- Called after `unit` was attacked by `other`.
function Script:onAfterAttacked(server, battle, unit, other)
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

return Script
