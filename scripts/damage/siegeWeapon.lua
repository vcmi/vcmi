local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Ballista deals additional damage based on hero attack
--- Only bonuses from hero itself (base stats) and from equipped artifacts are included
function Script:getBaseDamageSingle(info)
	local minDamage, maxDamage = Base.getBaseDamageSingle(self, info)

	if not self:hasBonusOfType(info.attackerBonuses, "SIEGE_WEAPON") then return minDamage, maxDamage end
	if info.attacker:isTurret() then return minDamage, maxDamage end

	-- only what the hero itself brings counts, so the two sources are asked after in turn
	local heroAttack = info.attacker:getBonusesValue({type = "PRIMARY_SKILL", subtype = "attack", sourceType = ENUM.BonusSource.artifact})
		+ info.attacker:getBonusesValue({type = "PRIMARY_SKILL", subtype = "attack", sourceType = ENUM.BonusSource.heroBaseSkill})

	return minDamage * (heroAttack + 1), maxDamage * (heroAttack + 1)
end

Script:declareBonus("SIEGE_WEAPON")

return Script
