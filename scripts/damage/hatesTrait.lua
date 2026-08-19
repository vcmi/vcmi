local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Hatred of a trait rather than of a creature - of everything that shoots, of everything undead.
--- Nothing of Heroes 3 hates that way; its hatreds name the creature, which the calculator handles.

function Script:bonusTypes()
	local types = Base.bonusTypes(self)

	table.insert(types, "HATES_TRAIT")

	return types
end

--- Hate of something the creature being struck happens to be.
function Script:getHateTraitFactor(info)
	if not self:carriesBonus(info.attackerBonuses, "HATES_TRAIT") then return 0 end

	return info.attacker:getBonuses({type = "HATES_TRAIT"}):filter(function(hate)
		-- the subtype of a hate names a bonus type, and any type at all may be hated - so this one
		-- question cannot be answered from the snapshot, which only speaks of declared types
		return info.defender:getBonuses({type = hate:getSubtype()}):size() > 0
	end):totalValue() / 100
end

function Script:getFactors(info)
	local factors = Base.getFactors(self, info)

	table.insert(factors, self:getHateTraitFactor(info))

	return factors
end

return Script
