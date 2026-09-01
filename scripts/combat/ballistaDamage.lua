local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- A war machine shoots for what the hero owning it is worth rather than for what its creature
--- says. The damage is settled once, when the battle is set up, and handed to the machine as a
--- bonus - nothing that feeds it can change while a battle runs, and a bonus is what every window,
--- tooltip and damage roll reads already.

--- Points of hero attack the machine profits from. Only what the hero is worth on its own and what
--- it wears counts; what an army, a spell or a terrain adds does not reach the machine.
function Script:getHeroAttack(unit)
	return unit:getBonusesValue({ type = "PRIMARY_SKILL", subtype = "attack", sourceType = ENUM.BonusSource.artifact })
		+ unit:getBonusesValue({ type = "PRIMARY_SKILL", subtype = "attack", sourceType = ENUM.BonusSource.heroBaseSkill })
end

--- Lowest and highest damage one creature of the machine deals, from what it deals on its own.
--- Overriding this is how a mod changes the formula; what is granted below is only the difference.
function Script:getDamageRange(unit, minDamage, maxDamage)
	local heroAttack = self:getHeroAttack(unit)

	return minDamage * (heroAttack + 1), maxDamage * (heroAttack + 1)
end

function Script:grantDamage(server, battle, unit, subtype, value)
	if value == 0 then return end

	server:addUnitBonus(battle, unit, {
		type       = "CREATURE_DAMAGE",
		subtype    = subtype,
		val        = value,
		duration   = ENUM.BonusDuration.oneBattle,
		sourceType = ENUM.BonusSource.creatureAbility,
		sourceID   = unit:getCreature():getJsonKey()
	}, false)
end

function Script:onBattleSetup(server, battle, unit, other)
	local minDamage = unit:getMinDamage(false)
	local maxDamage = unit:getMaxDamage(false)
	local newMin, newMax = self:getDamageRange(unit, minDamage, maxDamage)

	self:grantDamage(server, battle, unit, "creatureDamageMin", newMin - minDamage)
	self:grantDamage(server, battle, unit, "creatureDamageMax", newMax - maxDamage)
end

return Script
