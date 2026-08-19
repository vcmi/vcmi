local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Some creatures take more from a blow they never saw coming. Nothing of Heroes 3 does, so the
--- rule is kept apart from the calculator that would otherwise ask after it on every attack.

function Script:bonusTypes()
	local types = Base.bonusTypes(self)

	table.insert(types, "VULNERABLE_FROM_BACK")

	return types
end

--- How much more the target takes when the attacker had to turn around to reach it.
function Script:getFromBackFactor(info)
	local value = self:bonusValue(info.defender, info.defenderBonuses, "VULNERABLE_FROM_BACK")

	if value == 0 then return 0 end
	if not info.battle:isToReverse(info.attacker, info.defender, info.attackerHex, info.defenderHex) then return 0 end

	return value / 100
end

function Script:getFactors(info)
	local factors = Base.getFactors(self, info)

	table.insert(factors, self:getFromBackFactor(info))

	return factors
end

return Script
