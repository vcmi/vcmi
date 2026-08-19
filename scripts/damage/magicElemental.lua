local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Magic elementals deal half damage to what shrugs off high-level magic - black dragons and their
--- own kind. The rule names the creature outright, which is why it is kept apart from the rest.

local MAGIC_ELEMENTAL = "core:magicElemental"

function Script:getMagicFactor(info)
	-- the immunity is what makes this rare, and asking after it costs nothing
	if self:bonusValue(info.defender, info.defenderBonuses, "LEVEL_SPELL_IMMUNITY") < 5 then return 0 end
	if info.attacker:getCreature():getJsonKey() ~= MAGIC_ELEMENTAL then return 0 end

	return -0.5
end

Script:declareBonus("LEVEL_SPELL_IMMUNITY")
Script:addDamageFactor("getMagicFactor")

return Script
