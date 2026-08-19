local Script = setmetatable({}, {__index = Base})
Script.__index = Script

local function divideAndRound(dividend, divisor)
	local half = math.floor(divisor / 2)

	return math.floor((dividend + half - 1) / divisor)
end

--- HotA Nix ability: Ignores specified percentage of enemy Attack skill
function Script:getAttackIgnored(info, attackBase)
	if not self:hasBonusOfType(info.defenderBonuses, "ENEMY_ATTACK_REDUCTION") then return 0 end

	local reduction = info.defender:getBonusesValue({type = "ENEMY_ATTACK_REDUCTION", shooting = info.shooting})

	if reduction <= 0 then return 0 end

	return -math.min(divideAndRound(attackBase * reduction, 100), attackBase)
end

Script:declareBonus("ENEMY_ATTACK_REDUCTION")

return Script
