local Base = require("unitEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Returns the number of demons that can be raised from a dead unit.
function Script:raisedCreatureAmount(mechanics, unit)
	local creatureType = LIBRARY:getCreatureByName(self.id)

	local deadCount         = unit:getBaseAmount()
	local deadTotalHealth   = unit:getTotalHealth()
	local raisedMaxHealth   = creatureType:getMaxHealth()
	local raisedTotalHealth = mechanics:applySpellBonus(mechanics:getEffectValue(), unit)

	local maxFromHealth     = math.floor(deadTotalHealth   / raisedMaxHealth)
	local maxFromAmount     = deadCount
	local maxFromSpellpower = math.floor(raisedTotalHealth / raisedMaxHealth)

	return math.min(maxFromHealth, maxFromAmount, maxFromSpellpower)
end

--- A unit is a valid target if it is a dead, non-ghost corpse whose hexes are
--- not blocked by any alive unit, and our spellpower can raise at least one demon.
function Script:isValidTarget(mechanics, unit)
	if not unit:isDead() then return false end

	local hexes = unit:getHexes()
	for i = 1, hexes:size() do
		local hex = hexes:at(i)
		local blockers = mechanics:getBattle():getUnitsIf(function(other)
			return other:isValidTarget(false) and other:coversPos(hex)
		end)
		if #blockers > 0 then return false end
	end

	if unit:isGhost() then return false end

	if self:raisedCreatureAmount(mechanics, unit) == 0 then return false end

	return mechanics:isReceptive(unit)
end

--- Raise demons from each target corpse and remove the corpse.
function Script:apply(mechanics, server, target)
	local creatureType = LIBRARY:getCreatureByName(self.id)
	local battle       = mechanics:getBattle()

	for _, dest in ipairs(target) do
		local targetStack = dest.unit

		if targetStack == nil or not targetStack:isDead() or targetStack:isGhost() then
			print("DemonSummon: no valid corpse for demonization")
			break
		end

		local hex = battle:getAvailableHex(
			creatureType,
			mechanics:getCasterSide(),
			targetStack:getPosition()
		)

		if not hex:isValid() then
			print("DemonSummon: no available hex for summoned unit")
			break
		end

		local finalAmount = self:raisedCreatureAmount(mechanics, targetStack)

		if finalAmount < 1 then
			print("DemonSummon: raised zero creatures")
			break
		end

		server:addUnit(
			battle,
			{
				count    = finalAmount,
				type     = creatureType,
				side     = mechanics:getCasterSide(),
				position = hex,
				summoned = not self.permanent
			}
		)

		server:removeUnit(battle, targetStack)
	end
end

--- Returns the number of demons that would be raised for the hover preview.
function Script:getHealthChange(mechanics, spellTarget)
	local creatureType = LIBRARY:getCreatureByName(self.id)

	for _, dest in ipairs(spellTarget) do
		if dest.unit ~= nil then
			local amount = self:raisedCreatureAmount(mechanics, dest.unit)
			return {
				hpDelta    = 0,
				unitsDelta = amount,
				unitType   = creatureType
			}
		end
	end

	return { hpDelta = 0, unitsDelta = 0 }
end

return Script
