local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- War machine damage is scaled by the attack of the hero owning it instead of by its creature
--- stats. Calculated once, when the battle is set up, and granted to the machine as a bonus -
--- nothing it depends on can change during a battle, and every window, tooltip and damage roll
--- already reads bonuses.

--- Hero attack the machine profits from: only the hero's own skill and its equipped artifacts.
--- Attack granted by an army, a spell or the terrain does not count.
function Script:getHeroAttack(unit)
	return unit:getBonusesValue({ type = "PRIMARY_SKILL", subtype = "attack", sourceType = ENUM.BonusSource.artifact })
		+ unit:getBonusesValue({ type = "PRIMARY_SKILL", subtype = "attack", sourceType = ENUM.BonusSource.heroBaseSkill })
end

--- Lowest and highest damage of one creature of the machine, from its own damage. A mod changes
--- the formula by overriding this; only the difference is granted as a bonus below.
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
