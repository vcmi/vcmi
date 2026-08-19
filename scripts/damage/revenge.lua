local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Implements HotA Haspid abiity: Revenge - dealt damage is increased by how many Haspids from this unit were killed since the start of combat
function Script:getRevengeFactor(info)
	if not self:hasBonusOfType(info.attackerBonuses, "REVENGE") then return 0 end

	local attacker = info.attacker
	local creatureHealth = attacker:getMaxHealth()

	return math.sqrt((attacker:getBaseAmount() + 1) * creatureHealth / (attacker:getAvailableHealth() + creatureHealth) - 1)
end

Script:declareBonus("REVENGE")
Script:addDamageFactor("getRevengeFactor")

return Script
