local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Magic Elementals do half damage to units immune to magic (Black Dragons and other Magic Elementals)
local MAGIC_ELEMENTAL = "core:magicElemental"

function Script:getMagicFactor(info)
	if self:getBonusValueOfType(info.defender, info.defenderBonuses, "LEVEL_SPELL_IMMUNITY") < 5 then return 0 end
	if info.attacker:getCreature():getJsonKey() ~= MAGIC_ELEMENTAL then return 0 end

	return -0.5
end

Script:declareBonus("LEVEL_SPELL_IMMUNITY")
Script:addDamageFactor("getMagicFactor")

return Script
