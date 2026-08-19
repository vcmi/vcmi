local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- A stack that strikes harder the more of it has been killed - the HotA Haspid ability. Nothing of
--- Heroes 3 has it.

--- How much the losses of the attacker add to its blow.
function Script:getRevengeFactor(info)
	if not self:carriesBonus(info.attackerBonuses, "REVENGE") then return 0 end

	local attacker = info.attacker
	local creatureHealth = attacker:getMaxHealth()

	return math.sqrt((attacker:getBaseAmount() + 1) * creatureHealth / (attacker:getAvailableHealth() + creatureHealth) - 1)
end

Script:declareBonus("REVENGE")
Script:addDamageFactor("getRevengeFactor")

return Script
