local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Mod feature: creature receives additional damage when it is hit from the back (e.g. when it needs to turn around)
--- NOTE: ability has another hardcoded part - such creature will not turn around to face the attacker
function Script:getFromBackFactor(info)
	local value = self:getBonusValueOfType(info.defender, info.defenderBonuses, "VULNERABLE_FROM_BACK")

	if value == 0 then return 0 end
	if not info.battle:isToReverse(info.attacker, info.defender, info.attackerHex, info.defenderHex) then return 0 end

	return value / 100
end

Script:declareBonus("VULNERABLE_FROM_BACK")
Script:addDamageFactor("getFromBackFactor")

return Script
