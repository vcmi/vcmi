local Script = {}
Script.__index = Script
Script.type = "combatScript"

--- Base class for scripts attached to units via the COMBAT_EVENT_TRIGGER bonus.
--- Define handlers only for supported events.
---
--- The handlers, all of them `function Script:on<Event>(server, battle, unit, other, payload)`:
--- onBeforeAttack, onAfterAttack, onBeforeAttacked, onAfterAttacked, onWait, onDefend,
--- onBeforeMove, onAfterMove, onUnitSpellcast, onSpellHit, onDeath, onActionFinished,
--- onBattleSetup, onBattleStart, onRoundStart.
--- See docs/modders/Lua/Combat_Event_Scripts.md for what each of them means.
---
--- `unit` is the bonus bearer. `other` is the other event participant and may be nil.
--- Bonus parameters are read-only fields of `self`; persistent state requires a separate bonus.
---
--- Attack handlers execute for counterattacks, every hit of a multiple attack and dead bearers.
--- Handlers that require a living bearer must check `unit:isAlive()`.

--- Returns the `payload.targets` entry for `unit`, or nil when no matching target exists.
function Script:ownEntry(unit, payload)
	for _, target in ipairs(payload.targets or {}) do
		if target.unit and target.unit:unitID() == unit:unitID() then
			return target
		end
	end

	return nil
end

return Script
