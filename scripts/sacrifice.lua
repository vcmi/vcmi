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
	return HEAL_LEVEL_FROM_STRING[self.healLevel] or ENUM.HealLevel.resurrect
end
function Script:getHealPower()
	return HEAL_POWER_FROM_STRING[self.healPower] or ENUM.HealPower.permanent
end

--- Accept any unit (dead or alive) as a potential target.
function Script:isValidTarget(mechanics, unit)
	return unit:isValidTarget(true)
end

--- Enforce target type pair [CREATURE, CREATURE].
function Script:adjustTargetTypes(mechanics, types)
	if #types == 0 then return types end
	if types[1] ~= ENUM.AimType.creature then return {} end
	if #types == 1 then return { ENUM.AimType.creature, ENUM.AimType.creature } end
	if types[2] ~= ENUM.AimType.creature then return {} end
	return types
end

--- Require at least one dead unit AND one alive unit, both owner-matching.
function Script:applicableGeneral(mechanics, problem)
	local units = mechanics:getBattle():getUnitsIf(function(unit)
		return unit:isValidTarget(true) and mechanics:isReceptive(unit) and mechanics:ownerMatches(unit)
	end)

	local hasDeadTarget  = false
	local hasAliveVictim = false
	for _, unit in ipairs(units) do
		if unit:isDead()  then hasDeadTarget  = true end
		if unit:isAlive() then hasAliveVictim = true end
		if hasDeadTarget and hasAliveVictim then break end
	end

	if not (hasDeadTarget and hasAliveVictim) then
		problem:addStandard(mechanics, ENUM.SpellCastProblem.noAppropriateTarget)
		return false
	end
	return true
end

--- First target must be a dead unit; second must be an alive, receptive, owner-matching unit.
function Script:applicableTarget(mechanics, problem, target)
	if #target == 0 then return false end
	local deadUnit = target[1].unit
	if not deadUnit or deadUnit:isAlive() then return false end
	if #target < 2 then return true end
	local victim = target[2].unit
	if not victim or not victim:isAlive() then return false end
	if not mechanics:isReceptive(victim) then return false end
	if mechanics:isSmart() and not mechanics:ownerMatches(victim) then return false end
	return true
end

--- Filter the dead target via the base, then append a live victim from aimPoint.
function Script:transformTarget(mechanics, aimPoint, spellTarget)
	local filtered = Base.transformTarget(self, mechanics, aimPoint, spellTarget)
	if #filtered == 0 then return {} end
	local result = { filtered[1] }
	if #aimPoint >= 2 then
		local victim = aimPoint[2].unit
		if victim and victim:isAlive() and mechanics:isReceptive(victim)
				and mechanics:ownerMatches(victim) then
			result[2] = aimPoint[2]
		end
	end
	return result
end

function Script:calculateHealValue(mechanics, victim)
	return (mechanics:getEffectPower() + victim:getMaxHealth()
		+ mechanics:calculateRawEffectValue(0, 1)) * victim:getCount()
end

--- Returns HP change preview.
function Script:getHealthChange(mechanics, spellTarget)
	if #spellTarget == 0 then
		return { hpDelta = 0, unitsDelta = 0 }
	end
	local unit = spellTarget[1].unit
	if not unit then
		return { hpDelta = 0, unitsDelta = 0 }
	end
	if not unit:isAlive() then
		-- dead target: show maximum possible resurrection
		local baseAmount = unit:getBaseAmount()
		local maxHP      = unit:getMaxHealth()
		return {
			hpDelta   = baseAmount * maxHP,
			unitsDelta = baseAmount,
			unitType   = unit:getCreature()
		}
	else
		-- alive unit shown in UI as sacrifice victim
		return {
			hpDelta   = self:calculateHealValue(mechanics, unit),
			unitsDelta = -unit:getCount(),
			unitType   = unit:getCreature()
		}
	end
end

--- Heal the dead target with the sacrifice value then remove the victim.
function Script:apply(mechanics, server, target)
	if #target ~= 2 then return end
	local deadTarget = target[1].unit
	local victim     = target[2].unit
	if not deadTarget or not victim then return end

	local healValue = self:calculateHealValue(mechanics, victim)
	local battle    = mechanics:getBattle()

	local _, resurrected = server:healUnit(
		battle, deadTarget, healValue,
		self:getHealLevel(), self:getHealPower())

	server:removeUnit(battle, victim)

	if resurrected > 0 then
		local textID = resurrected == 1 and "core.genrltxt.117" or "core.genrltxt.116"
		local nameTextID = deadTarget:getCreature():getNameTextID(deadTarget:getCount())
		server:appendLog(battle, {
			append         = { textID },
			replaceStrings = { nameTextID },
			replaceNumbers = { resurrected }
		})
	end
end

return Script
