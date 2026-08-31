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

return Script
