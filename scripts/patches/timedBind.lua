-- Patch over `timed`: stores the caster's unit ID in BIND_EFFECT.addInfo when bind is cast
-- by a unit (not a hero), so the engine can later locate the binding unit.
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:convertBonuses(mechanics)
	local converted = Base.convertBonuses(self, mechanics)
	if mechanics:getSpell():getJsonKey() == "core:bind" and not mechanics:getHeroCaster() then
		for _, nb in pairs(converted) do
			if nb.type == "BIND_EFFECT" then
				nb.addInfo = mechanics:getUnitCaster():unitID()
			end
		end
	end
	return converted
end

return Script
