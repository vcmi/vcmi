local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Psychic elementals deal half damage to what has no mind to attack. The rule names the creature
--- outright, which is why it is kept apart from the rest.

local PSYCHIC_ELEMENTAL = "core:psychicElemental"

function Script:bonusTypes()
	local types = Base.bonusTypes(self)

	table.insert(types, "MIND_IMMUNITY")

	return types
end

function Script:getMindFactor(info)
	if not self:carriesBonus(info.defenderBonuses, "MIND_IMMUNITY") then return 0 end
	if info.attacker:getCreature():getJsonKey() ~= PSYCHIC_ELEMENTAL then return 0 end

	return 0.5
end

function Script:getFactors(info)
	local factors = Base.getFactors(self, info)

	table.insert(factors, -self:getMindFactor(info))

	return factors
end

return Script
