local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Magic elementals deal half damage to what shrugs off high-level magic - black dragons and their
--- own kind. The rule names the creature outright, which is why it is kept apart from the rest.

local MAGIC_ELEMENTAL = "core:magicElemental"

function Script:bonusTypes()
	local types = Base.bonusTypes(self)

	table.insert(types, "LEVEL_SPELL_IMMUNITY")

	return types
end

function Script:getMagicFactor(info)
	-- the immunity is what makes this rare, and asking after it costs nothing
	if self:bonusValue(info.defender, info.defenderBonuses, "LEVEL_SPELL_IMMUNITY") < 5 then return 0 end
	if info.attacker:getCreature():getJsonKey() ~= MAGIC_ELEMENTAL then return 0 end

	return 0.5
end

function Script:getFactors(info)
	local factors = Base.getFactors(self, info)

	table.insert(factors, -self:getMagicFactor(info))

	return factors
end

return Script
