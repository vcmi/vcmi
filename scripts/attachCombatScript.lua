local Base = require("unitEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Grants every target a COMBAT_EVENT_TRIGGER bonus that runs the configured combat script.
--- The caster is gone by the time the script actually runs, so everything the script may need
--- from it is snapshotted into the parameters now.
function Script:buildParameters(mechanics)
	local parameters = {}

	for key, value in pairs(self.eventParameters or {}) do
		parameters[key] = value
	end

	parameters.spell = mechanics:getSpell():getJsonKey()
	parameters.casterPower = mechanics:getEffectPower()
	parameters.casterLevel = mechanics:getEffectLevel()
	parameters.casterSide = mechanics:getCasterSide()

	return parameters
end

function Script:apply(mechanics, server, target)
	local battle = mechanics:getBattle()
	local spellKey = mechanics:getSpell():getJsonKey()
	local parameters = self:buildParameters(mechanics)

	for _, dest in ipairs(target) do
		local unit = dest.unit
		if unit and unit:isAlive() then
			server:addUnitBonus(battle, unit, {
				type = "COMBAT_EVENT_TRIGGER",
				duration = "N_TURNS",
				turns = mechanics:getEffectDuration(),
				sourceType = "SPELL_EFFECT",
				sourceID = spellKey,
				stacking = spellKey,
				addInfo = {
					eventScript = self.eventScript,
					eventParameters = parameters
				}
			}, false)
		end
	end
end

return Script
