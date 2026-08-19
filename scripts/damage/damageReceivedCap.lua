local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- A ceiling on what one blow may take off a stack, whatever the blow is worth. Nothing of Heroes 3
--- carries it.

--- Most damage the target can take from one blow, as a share of the health of one creature.
function Script:getDamageCap(info)
	local percentage = self:bonusValue(info.defender, info.defenderBonuses, "DAMAGE_RECEIVED_CAP")

	if percentage <= 0 then return Base.getDamageCap(self, info) end

	return math.floor(info.defender:getMaxHealth() * percentage / 100)
end

Script:declareBonus("DAMAGE_RECEIVED_CAP")

return Script
