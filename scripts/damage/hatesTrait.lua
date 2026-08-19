local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Support for "Creature deals increased damage to creatures with specific bonus"
--- Mod only, not used by Heroes 3
function Script:getHateTraitFactor(info)
	if not self:hasBonusOfType(info.attackerBonuses, "HATES_TRAIT") then return 0 end

	return info.attacker:getBonuses({type = "HATES_TRAIT"}):filter(function(hate)
		-- the subtype of a hate names a bonus type, and any type at all may be hated - so this one
		-- question cannot be answered from the snapshot, which only speaks of declared types
		return info.defender:hasBonuses({type = hate:getSubtype()})
	end):totalValue() / 100
end

Script:declareBonus("HATES_TRAIT")
Script:addDamageFactor("getHateTraitFactor")

return Script
