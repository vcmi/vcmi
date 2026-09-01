local Base = require("spells/unitEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Grants every target a COMBAT_EVENT_TRIGGER bonus that runs the configured combat script.
--- The bonus carries `eventParameters` unchanged, so the script sees exactly what the spell
--- declared for it - anything it needs to know about the caster has to be passed there too.
function Script:apply(mechanics, server, target)
	local battle = mechanics:getBattle()
	local spellKey = mechanics:getSpell():getJsonKey()

	for _, dest in ipairs(target) do
		local unit = dest.unit
		if unit and unit:isAlive() then
			server:addUnitBonus(battle, unit, {
				type = "COMBAT_EVENT_TRIGGER",
				subtype = self.eventScript,
				val = self.eventValue or 0,
				duration = "N_TURNS",
				turns = mechanics:getEffectDuration(),
				sourceType = "SPELL_EFFECT",
				sourceID = spellKey,
				stacking = spellKey,
				addInfo = self.eventParameters
			}, false)
		end
	end
end

return Script
