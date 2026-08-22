local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- HotA change of skills: The War Machine cannot be dealt more than 40% of its Health as Damage from any single damage source. 
function Script:getDamageCap(info)
	local percentage = self:getBonusValueOfType(info.defender, info.defenderBonuses, "DAMAGE_RECEIVED_CAP")

	if percentage <= 0 then return Base.getDamageCap(self, info) end

	return math.floor(info.defender:getMaxHealth() * percentage / 100)
end

Script:declareBonus("DAMAGE_RECEIVED_CAP")

return Script
