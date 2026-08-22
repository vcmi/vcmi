local Script = {}
Script.__index = Script
Script.type = "combatScript"

--- Base class for scripts attached to units via the COMBAT_EVENT_TRIGGER bonus.
--- A script defines a method only for the events it reacts to; an event whose method is absent is
--- never handed to the script at all.
---
--- The handlers, all of them `function Script:on<Event>(server, battle, unit, other, payload)`:
--- onBeforeAttack, onAfterAttack, onBeforeAttacked, onAfterAttacked, onWait, onDefend,
--- onBeforeMove, onAfterMove, onUnitSpellcast, onBattleSetup, onBattleStart, onRoundStart.
--- See docs/modders/Lua/Combat_Event_Scripts.md for what each of them means.
---
--- In every handler `unit` is the bearer of the bonus and `other` is the unit on the opposite
--- side of the event (the attacker, the victim, ...), which may be nil.
--- Parameters stored in the bonus are available as fields on `self` and are read-only;
--- a script that needs to remember something across events must store it itself,
--- for example in a bonus of its own.
---
--- No event is withheld: the attack handlers fire for a counterattack, for every blow of a
--- multiple attack, and even when the bearer died while the attack was resolving. Whether that is
--- a reason to do nothing is the script's own call - a reflecting ability answers a lethal blow
--- while dying, while an ability that strikes back must first check `unit:isAlive()`.

return Script
