local Base = require("spells/timed")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Already checked everything
function Script:applicableTarget(mechanics, problem, target)
	return true
end

--- Add the caster, hex does not matter
function Script:transformTarget(mechanics, aimPoint, spellTarget)
	return {{ unit = mechanics:getUnitCaster(), hex = nil }}
end

--- Only for unit-self cast
function Script:applicableGeneral(mechanics, problem)
	local caster = mechanics:getUnitCaster()
	if caster and caster:isAlive() then
		return true
	end

	problem:addStandard(mechanics, ENUM.SpellCastProblem.noAppropriateTarget)
	return false
end

function Script:apply(mechanics, server, target)
	local battle   = mechanics:getBattle()
	local describe = server:describeChanges()
	local converted = self:convertBonuses(mechanics)
	local unit = target[1].unit

	local buffer = {}
	for name, nb in pairs(converted) do
		buffer[name] = self:deepCopyBonus(nb)
	end

	if describe then
		self:describeEffect(server, battle, unit)
	end

	for _, nb in pairs(buffer) do
		server:addUnitBonus(battle, unit, nb, self.cumulative or false)
	end
end

function Script:describeEffect(server, battle, unit)
	if not self.battleLogPlural or self.battleLogPlural == "" then return end
	local count = unit:getCount()
	local textID = (self.battleLogSingular and self.battleLogSingular ~= "" and count == 1) and self.battleLogSingular or self.battleLogPlural
	local nameTextID = unit:getCreature():getNameTextID(count)
	server:appendLog(battle, {
		append         = { textID },
		replaceStrings = { nameTextID }
	})
end

return Script
