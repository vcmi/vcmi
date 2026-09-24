local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Applies war-machine damage calculated from the owning hero's base and artifact attack during battle setup.

--- Returns hero base attack plus artifact attack.
function Script:getHeroAttack(unit)
	return unit:getBonusesValue({ type = "PRIMARY_SKILL", subtype = "attack", sourceType = ENUM.BonusSource.artifact })
		+ unit:getBonusesValue({ type = "PRIMARY_SKILL", subtype = "attack", sourceType = ENUM.BonusSource.heroBaseSkill })
end

--- Returns scaled per-creature damage. Override to replace the formula.
function Script:getDamageRange(unit, minDamage, maxDamage)
	local heroAttack = self:getHeroAttack(unit)

	return minDamage * (heroAttack + self.val), maxDamage * (heroAttack + self.val)
end

function Script:grantDamage(server, battle, unit, subtype, value)
	if value == 0 then return end

	server:addUnitBonus(battle, unit, {
		type       = "CREATURE_DAMAGE",
		subtype    = subtype,
		val        = value,
		valueType  = ENUM.BonusValueType.independentMax,
		duration   = ENUM.BonusDuration.oneBattle,
		sourceType = ENUM.BonusSource.creatureAbility,
		sourceID   = unit:getCreature():getJsonKey()
	}, false)
end

function Script:onBattleSetup(server, battle, unit, other)
	local minDamage = unit:getMinDamage(false)
	local maxDamage = unit:getMaxDamage(false)
	local newMin, newMax = self:getDamageRange(unit, minDamage, maxDamage)

	self:grantDamage(server, battle, unit, "creatureDamageMin", newMin)
	self:grantDamage(server, battle, unit, "creatureDamageMax", newMax)
end

return Script
