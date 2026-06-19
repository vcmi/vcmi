local Base = require("unitEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

local HEAL_LEVEL_FROM_STRING = {
	heal      = ENUM.HealLevel.heal,
	resurrect = ENUM.HealLevel.resurrect,
	overHeal  = ENUM.HealLevel.overheal,
}
local HEAL_POWER_FROM_STRING = {
	oneBattle = ENUM.HealPower.oneBattle,
	permanent = ENUM.HealPower.permanent,
}

function Script:getHealLevel()
	return HEAL_LEVEL_FROM_STRING[self.healLevel] or ENUM.HealLevel.heal
end
function Script:getHealPower()
	return HEAL_POWER_FROM_STRING[self.healPower] or ENUM.HealPower.permanent
end
function Script:getMinFullUnits()
	return self.minFullUnits or 0
end

--- Only injured units are valid; resurrect/overheal levels also accept dead units.
function Script:isValidTarget(mechanics, unit)
	local level = self:getHealLevel()
	local allowDead = level ~= ENUM.HealLevel.heal
	if not unit:isValidTarget(allowDead) then return false end

	local injuries = unit:getTotalHealth() - unit:getAvailableHealth()
	if injuries <= 0 then return false end

	local mfu = self:getMinFullUnits()
	if mfu > 0 then
		local hpGained = math.min(mechanics:getEffectValue(), injuries)
		if hpGained < mfu * unit:getMaxHealth() then return false end
	end

	if unit:isDead() then
		local hexes = unit:getHexes()
		for i = 1, hexes:size() do
			local hex = hexes:at(i)
			local blockers = mechanics:getBattle():getUnitsIf(function(other)
				return other ~= unit and other:isValidTarget(false) and other:coversPos(hex)
			end)
			if #blockers > 0 then return false end
		end
	end

	return true
end

--- Returns HP and unit count change for hover tooltip.
function Script:getHealthChange(mechanics, spellTarget)
	local result = { hpDelta = 0, unitsDelta = 0 }
	for _, dest in ipairs(spellTarget) do
		local unit = dest.unit
		if unit then
			local copy = unit:copy()
			local healedHP, resurrected = copy:heal(mechanics:getEffectValue(), self:getHealLevel(), self:getHealPower())
			result.hpDelta   = result.hpDelta   + healedHP
			result.unitsDelta = result.unitsDelta + resurrected
			result.unitType  = unit:getCreature()
		end
	end
	return result
end

--- Heals each target unit and emits battle log messages.
function Script:apply(mechanics, server, target)
	local battle       = mechanics:getBattle()
	local isUnitCaster = mechanics:getHeroCaster() == nil

	for _, dest in ipairs(target) do
		local unit = dest.unit
		if unit then
			local healedHP, resurrected = server:healUnit(
				battle, unit, mechanics:getEffectValue(), self:getHealLevel(), self:getHealPower())

			if resurrected > 0 then
				local textID = resurrected == 1 and "core.genrltxt.117" or "core.genrltxt.116"
				local nameTextID = unit:getCreature():getNameTextID(unit:getCount())
				server:appendLog(battle, {
					append         = { textID },
					replaceStrings = { nameTextID },
					replaceNumbers = { resurrected }
				})
			elseif healedHP > 0 and isUnitCaster then
				local casterUnit = mechanics:getUnitCaster()
				server:appendLog(battle, {
					append         = { "core.genrltxt.414" },
					replaceStrings = {
						casterUnit:getCreature():getNameTextID(1),
						unit:getCreature():getNameTextID(1)
					},
					replaceNumbers = { healedHP }
				})
			end
		end
	end
end

return Script
