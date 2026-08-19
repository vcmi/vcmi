local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- A ballista fires for its own damage times the attack of the hero owning it. Towers carry the same
--- bonus and are not affected - what they shoot for is decided by the town.

function Script:getBaseDamageSingle(info)
	local minDamage, maxDamage = Base.getBaseDamageSingle(self, info)

	if not self:carriesBonus(info.attackerBonuses, "SIEGE_WEAPON") then return minDamage, maxDamage end
	if info.attacker:isTurret() then return minDamage, maxDamage end

	-- only what the hero itself brings counts, so the two sources are asked after in turn
	local heroAttack = info.attacker:getBonusesValue({type = "PRIMARY_SKILL", subtype = "attack", sourceType = ENUM.BonusSource.artifact})
		+ info.attacker:getBonusesValue({type = "PRIMARY_SKILL", subtype = "attack", sourceType = ENUM.BonusSource.heroBaseSkill})

	return minDamage * (heroAttack + 1), maxDamage * (heroAttack + 1)
end

Script:declareBonus("SIEGE_WEAPON")

return Script
