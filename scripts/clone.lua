local Base = require("unitEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:isReceptive(mechanics, unit)
	if unit:getCreature():getLevel() > self.maxTier then return false end
	return Base.isReceptive(self, mechanics, unit)
end

function Script:isValidTarget(mechanics, unit)
	if unit:isClone()  then return false end
	if unit:hasClone() then return false end
	return unit:isValidTarget(false)
end

function Script:apply(mechanics, server, target)
	local battle = mechanics:getBattle()
	for _, dest in ipairs(target) do
		local unit = dest.unit
		if unit == nil then goto continue end
		if unit:getCount() < 1 then goto continue end

		local creature = unit:getCreature()
		local hex = battle:getAvailableHex(
			creature,
			mechanics:getCasterSide(),
			unit:getPosition()
		)
		if not hex:isValid() then break end

		local cloneUnit = server:addUnit(battle, {
			count    = unit:getCount(),
			type     = creature,
			side     = mechanics:getCasterSide(),
			position = hex,
			summoned = true
		})
		if cloneUnit == nil then break end

		local cloneState = cloneUnit:copy()
		cloneState:setCloned(true)
		server:changeUnit(battle, cloneState)

		local originalState = unit:copy()
		originalState:setClone(cloneUnit)
		server:changeUnit(battle, originalState)

		server:addUnitBonus(battle, cloneUnit, {
			duration   = ENUM.BonusDuration.nTurns,
			type       = "NONE",
			sourceType = ENUM.BonusSource.spellEffect,
			val        = 0,
			sourceID   = mechanics:getSpell():getJsonKey(),
			turns      = mechanics:getEffectDuration()
		}, true)

		::continue::
	end
end

return Script
