-- Patch over `timed`: inverts GENERAL_DAMAGE_REDUCTION value for shield-family spells,
-- so that the JSON-side number is the % of damage blocked rather than the % passed through.
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:convertBonuses(mechanics)
	local converted = Base.convertBonuses(self, mechanics)
	local spellKey = mechanics:getSpell():getJsonKey()
	if spellKey == "core:shield" or spellKey == "core:airShield" then
		for _, nb in pairs(converted) do
			if nb.type == "GENERAL_DAMAGE_REDUCTION" then
				nb.val = 100 - (nb.val or 0)
			end
		end
	end
	return converted
end

return Script
