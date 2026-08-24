local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Psychic Elementals do half damage to units immune to Mind spells
local PSYCHIC_ELEMENTAL = "core:psychicElemental"

function Script:getMindFactor(info)
	if not self:hasBonusOfType(info.defenderBonuses, "MIND_IMMUNITY") then return 0 end
	if info.attacker:getCreature():getJsonKey() ~= PSYCHIC_ELEMENTAL then return 0 end

	return -0.5
end

Script:declareBonus("MIND_IMMUNITY")
Script:addDamageFactor("getMindFactor")

return Script
