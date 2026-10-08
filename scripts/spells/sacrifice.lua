local Base = require("spells/unitEffect")
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
	return HEAL_LEVEL_FROM_STRING[self.healLevel] or ENUM.HealLevel.resurrect
end
function Script:getHealPower()
	return HEAL_POWER_FROM_STRING[self.healPower] or ENUM.HealPower.permanent
end

--- Unit to heal: an injured unit, or a corpse not covered by an alive unit if the heal level allows resurrection.
function Script:isValidTarget(mechanics, unit)
	if not unit:isValidTarget(true) then return false end
	if unit:getAvailableHealth() >= unit:getTotalHealth() then return false end
	if unit:isDead() then
		if self:getHealLevel() == ENUM.HealLevel.heal then return false end
		local battle = mechanics:getBattle()
		local hexes = unit:getHexes()
		for i = 1, hexes:size() do
			if battle:getUnitByPos(hexes:at(i), true) ~= nil then return false end
		end
	end
	return true
end

--- Enforce target type pair [CREATURE, CREATURE].
function Script:adjustTargetTypes(mechanics, types)
	if #types == 0 then return types end
	if types[1] ~= ENUM.AimType.creature then return {} end
	if #types == 1 then return { ENUM.AimType.creature, ENUM.AimType.creature } end
	if types[2] ~= ENUM.AimType.creature then return {} end
	return types
end

--- Require a unit to heal and a different alive unit to sacrifice, both owner-matching.
function Script:applicableGeneral(mechanics, problem)
	local units = mechanics:getBattle():getUnitsIf(function(unit)
		return unit:isValidTarget(true) and mechanics:isReceptive(unit) and mechanics:ownerMatches(unit)
	end)

	local aliveCount = 0
	local hasAliveTarget = false
	local hasDeadTarget = false
	for _, unit in ipairs(units) do
		local isTarget = self:isValidTarget(mechanics, unit)
		if unit:isAlive() then
			aliveCount = aliveCount + 1
			hasAliveTarget = hasAliveTarget or isTarget
		else
			hasDeadTarget = hasDeadTarget or isTarget
		end
	end

	if (hasDeadTarget and aliveCount >= 1) or (hasAliveTarget and aliveCount >= 2) then
		return true
	end
	problem:addStandard(mechanics, ENUM.SpellCastProblem.noAppropriateTarget)
	return false
end

--- First target is the unit to heal; the second one, absent while it is not chosen yet, is a different alive unit to sacrifice.
function Script:applicableTarget(mechanics, problem, target)
	if #target == 0 then return false end
	local healed = target[1].unit
	if not healed or not self:isValidTarget(mechanics, healed) then return false end
	if #target == 1 then return true end
	local victim = target[2].unit
	if not victim or not victim:isAlive() then return false end
	if victim:unitID() == healed:unitID() then return false end
	if not mechanics:isReceptive(victim) then return false end
	if mechanics:isSmart() and not mechanics:ownerMatches(victim) then return false end
	return true
end

--- Resolve the unit to heal via the base, then the unit to sacrifice from the second aim point.
--- The second target is kept even without a unit, so that applicableTarget rejects it instead of treating the cast as one-target.
function Script:transformTarget(mechanics, aimPoint, spellTarget)
	local filtered = Base.transformTarget(self, mechanics, aimPoint, spellTarget)
	if #filtered == 0 then return {} end
	local result = { filtered[1] }
	if #aimPoint >= 2 then
		local dest = aimPoint[2]
		local victim = dest.unit or mechanics:getBattle():getUnitByPos(dest.hex, true)
		result[2] = { unit = victim, hex = dest.hex }
	end
	return result
end

function Script:calculateHealValue(mechanics, victim)
	return (mechanics:getEffectPower() + victim:getMaxHealth()
		+ mechanics:calculateRawEffectValue(0, 1)) * victim:getCount()
end

--- Returns the health restored to the first target: by sacrificing the second target, or all missing health while it is not chosen.
function Script:getHealthChange(mechanics, spellTarget)
	local healed = spellTarget[1] and spellTarget[1].unit
	if not healed then
		return { hpDelta = 0, unitsDelta = 0 }
	end

	local state = healed:copy()
	local amount = state:getTotalHealth() - state:getAvailableHealth()
	local victim = spellTarget[2] and spellTarget[2].unit
	if victim then
		amount = self:calculateHealValue(mechanics, victim)
	end

	local healedHP, resurrected = state:heal(amount, self:getHealLevel(), self:getHealPower())
	return {
		hpDelta    = healedHP,
		unitsDelta = resurrected,
		unitType   = healed:getCreature()
	}
end

--- Heal the first target with the sacrifice value of the second one, then remove the second one.
function Script:apply(mechanics, server, target)
	if #target ~= 2 then return end
	local healed = target[1].unit
	local victim = target[2].unit
	if not healed or not victim then return end

	local healValue = self:calculateHealValue(mechanics, victim)
	local battle    = mechanics:getBattle()

	local _, resurrected = server:healUnit(
		battle, healed, healValue,
		self:getHealLevel(), self:getHealPower())

	server:removeUnit(battle, victim)

	if resurrected > 0 then
		local textID = resurrected == 1 and "core.genrltxt.117" or "core.genrltxt.116"
		local nameTextID = healed:getCreature():getNameTextID(healed:getCount())
		server:appendLog(battle, {
			append         = { textID },
			replaceStrings = { nameTextID },
			replaceNumbers = { resurrected }
		})
	end
end

return Script
