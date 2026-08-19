local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Lowering the attack of whoever strikes you, the mirror of what a behemoth does to defence.
--- Nothing of Heroes 3 does it, so the rule is kept out of the calculator.

--- The engine's own rounding of a division: not quite to nearest, it takes half a step less.
local function divideAndRound(dividend, divisor)
	local half = math.floor(divisor / 2)

	return math.floor((dividend + half - 1) / divisor)
end

--- Attack the target makes its attacker lose, as a negative number.
function Script:getAttackIgnored(info, attackBase)
	if not self:carriesBonus(info.defenderBonuses, "ENEMY_ATTACK_REDUCTION") then return 0 end

	-- a bonus limited to melee is simply absent from a shot, and the other way round
	local reduction = info.defender:getBonuses({type = "ENEMY_ATTACK_REDUCTION"}):filter(function(bonus)
		local range = bonus:getEffectRange()

		if range == ENUM.BonusLimitEffect.noLimit then return true end
		if info.shooting then return range == ENUM.BonusLimitEffect.onlyDistanceFight end

		return range == ENUM.BonusLimitEffect.onlyMeleeFight
	end):totalValue()

	if reduction <= 0 then return 0 end

	return -math.min(divideAndRound(attackBase * reduction, 100), attackBase)
end

Script:declareBonus("ENEMY_ATTACK_REDUCTION")

return Script
