local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Writes down what the API answered, as the value of a bonus, so that a test can read it back.
local function record(server, battle, unit, bonusType, value)
	server:addUnitBonus(battle, unit, {
		type       = bonusType,
		sourceType = ENUM.BonusSource.other,
		val        = value,
		duration   = ENUM.BonusDuration.oneBattle
	}, true)
end

--- Luck and morale as the engine answers them, which is not the sum of the bonuses granting them.
function Script:onBattleStart(server, battle, unit, other)
	record(server, battle, unit, "PROBE_LUCK", unit:getLuck())
	record(server, battle, unit, "PROBE_MORALE", unit:getMorale())
end

--- Whether the unit would answer the blow about to land on it, and that an attack names no spell.
function Script:onBeforeAttacked(server, battle, unit, other, payload)
	if unit:ableToRetaliate() then
		record(server, battle, unit, "PROBE_FLAGS", 1)
	end

	if payload.spell then
		record(server, battle, unit, "PROBE_FLAGS", 4)
	end
end

--- The spell the unit just cast, which only the spellcast event names.
function Script:onUnitSpellcast(server, battle, unit, other, payload)
	if payload.spell and payload.spell:getJsonKey() == self.spell then
		record(server, battle, unit, "PROBE_FLAGS", 2)
	end
end

--- Movement of the unit, counted so that an action that walks before doing something else is
--- seen to have walked.
function Script:onAfterMove(server, battle, unit, other, payload)
	record(server, battle, unit, "PROBE_MOVES", 1)
end

--- A spell landed on this unit: who cast it, which one it was, and what the unit was before.
function Script:onSpellHit(server, battle, unit, other, payload)
	record(server, battle, unit, "PROBE_SPELL_HITS", 1)

	if not other then
		record(server, battle, unit, "PROBE_HERO_CASTS", 1)
	end

	if payload.spell and payload.spell:getJsonKey() == self.hitBy then
		record(server, battle, unit, "PROBE_SPELL_NAMED", 1)
	end

	local entry = self:ownEntry(unit, payload)
	if entry and entry.unitBefore then
		record(server, battle, unit, "PROBE_HEALTH_BEFORE", entry.unitBefore:getAvailableHealth())
		record(server, battle, unit, "PROBE_SPELL_DAMAGE", entry.damage)
	end
end

--- The action that reached this unit is over. Counted rather than flagged, so that a test can see
--- that it is one report per action rather than one per blow of it.
function Script:onActionFinished(server, battle, unit, other)
	record(server, battle, unit, "PROBE_ACTIONS", 1)

	if other and other:unitID() == unit:unitID() then
		record(server, battle, unit, "PROBE_OWN_ACTIONS", 1)
	end
end

return Script
