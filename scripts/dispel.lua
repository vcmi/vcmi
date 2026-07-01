local Base = require("unitEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:getDispelableBonuses(mechanics, unit)
	local currentSpellKey = mechanics:getSpell():getJsonKey()
	return unit:getBonuses(function(bonus)
		if bonus:getSource() ~= ENUM.BonusSource.spellEffect then return false end
		if bonus:getSourceID() == currentSpellKey then return false end
		local sourceSpell = LIBRARY:getSpellByName(bonus:getSourceID())
		if not sourceSpell then return false end
		if sourceSpell:isPersistent() then return false end
		if sourceSpell:isAdventure()  then return false end
		if self.dispelPositive and sourceSpell:isPositive() then return true end
		if self.dispelNegative and sourceSpell:isNegative() then return true end
		if self.dispelNeutral  and sourceSpell:isNeutral()  then return true end
		return false
	end)
end

function Script:isValidTarget(mechanics, unit)
	if not unit:isValidTarget(false) then return false end
	return self:getDispelableBonuses(mechanics, unit):size() > 0
end

function Script:apply(mechanics, server, target)
	local battle       = mechanics:getBattle()
	local positiveOnly = self.dispelPositive and not self.dispelNegative and not self.dispelNeutral

	for _, dest in ipairs(target) do
		local unit = dest.unit
		if unit then
			local bonuses = self:getDispelableBonuses(mechanics, unit)
			if bonuses:size() > 0 then
				if positiveOnly and server:describeChanges() then
					server:appendLog(battle, {
						append         = { "core.genrltxt.555" },
						replaceStrings = { unit:getCreature():getNameTextID(0) }
					})
				end
				server:removeUnitBonuses(battle, unit, bonuses)
			end
		end
	end
end

return Script
