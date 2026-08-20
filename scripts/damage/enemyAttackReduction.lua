local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- HotA Nix ability: Ignores specified percentage of enemy Attack skill
function Script:getAttackIgnored(info, attackBase)
	if not self:hasBonusOfType(info.defenderBonuses, "ENEMY_ATTACK_REDUCTION") then return 0 end

	local reduction = info.defender:getBonusesValue({type = "ENEMY_ATTACK_REDUCTION", shooting = info.shooting})

	if reduction <= 0 then return 0 end

	-- Hota rounds down what is left of the attack, so the ignored share rounds up
	local kept = math.max(0, math.floor(attackBase * (100 - reduction) / 100))

	return kept - attackBase
end

Script:declareBonus("ENEMY_ATTACK_REDUCTION")

return Script
