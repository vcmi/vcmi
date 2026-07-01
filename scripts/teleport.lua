local Base = require("unitEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Enforce that target types are exactly [creature, location].
--- The spell config declares "targetType": "CREATURE"; this adds the destination hex.
function Script:adjustTargetTypes(mechanics, types)
	if #types == 0 then return types end
	if types[1] ~= ENUM.AimType.creature then return {} end
	if #types == 1 then return { ENUM.AimType.creature, ENUM.AimType.location } end
	if types[2] ~= ENUM.AimType.location then return {} end
	return types
end

--- Validate the destination hex (two-target phase only).
--- Single-target phase (unit hover) delegates to the base UnitEffect check.
function Script:applicableTarget(mechanics, problem, target)
	if #target <= 1 then
		return Base.applicableTarget(self, mechanics, problem, target)
	end

	local unit    = target[1].unit
	local fromHex = target[1].hex
	local toHex   = target[2].hex

	if not unit then
		problem:addStandard(mechanics, ENUM.SpellCastProblem.wrongSpellTarget)
		return false
	end

	if not toHex:isValid() or not mechanics:getBattle():isAccessibleForUnit(unit, toHex) then
		problem:addStandard(mechanics, ENUM.SpellCastProblem.wrongSpellTarget)
		return false
	end

	if mechanics:getBattle():hasPenaltyOnLine(fromHex, toHex, not self.isWallPassable, not self.isMoatPassable) then
		problem:addStandard(mechanics, ENUM.SpellCastProblem.wrongSpellTarget)
		return false
	end

	return true
end

--- Filter the unit target via the base, then re-attach the destination hex.
function Script:transformTarget(mechanics, aimPoint, spellTarget)
	local filtered = Base.transformTarget(self, mechanics, aimPoint, spellTarget)
	if #filtered == 0 then return {} end
	if #aimPoint >= 2 then
		filtered[2] = aimPoint[2]
	end
	return filtered
end

--- Move the unit to the destination hex via a teleport (no path, no distance cost).
function Script:apply(mechanics, server, target)
	local unit    = target[1].unit
	local destHex = target[2].hex
	server:moveUnit(mechanics:getBattle(), unit, destHex, true)
end

return Script
