local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Exercises the battle queries that take explicit hexes, which is the only way to ask them about an
--- attack that has not happened yet. Reports by damaging its own bearer: 1 point when every check
--- passed, 1 + the number of the first failed check otherwise.

--- A hex far enough from anything to be out of shooting range.
local function farAway(hex)
	for _ = 1, 12 do
		hex = hex:copyToWest()
	end

	return hex
end

--- First bonus of the given type carried by the unit, or nil.
local function bonusOfType(unit, type)
	local found = unit:getBonusesOfType(type)

	if found:size() == 0 then return nil end

	return found:getBonus(1)
end

local function hasBonusOfType(unit, type)
	return bonusOfType(unit, type) ~= nil
end

function Script:onAfterAttack(server, battle, unit, other, payload)
	if not other then return end

	local meleeOnly = bonusOfType(unit, "PERCENTAGE_DAMAGE_BOOST")
	local hatesTrait = bonusOfType(unit, "HATES_TRAIT")

	local far = farAway(unit:getPosition())
	local eastOfTarget = other:getPosition():copyToEast()
	local westOfTarget = other:getPosition():copyToWest()

	local checks = {
		-- the two stacks stand next to each other, so nothing is out of range
		battle:hasDistancePenalty(unit, other) == false,
		-- ... but the shot is a long one from the far hex, which is what proves the argument is read
		battle:hasDistancePenalty(unit, other, far) == true,
		-- an explicit nil has to fall back to where the shooter stands, leaving only the target moved
		battle:hasDistancePenalty(unit, other, nil, far) == true,
		battle:hasWallPenalty(unit, other) == false,
		-- one of the two sides of the target is behind the attacker, whichever way it faces
		battle:isToReverse(unit, other, eastOfTarget) ~= battle:isToReverse(unit, other, westOfTarget),
		unit:getFirstHPleft() == unit:getMaxHealth(),
		unit:isTurret() == false,
		unit:isShooter() == false,
		-- a bonus that only counts in melee says so, one that counts everywhere says that
		meleeOnly ~= nil,
		meleeOnly ~= nil and meleeOnly:getEffectRange() == ENUM.BonusLimitEffect.onlyMeleeFight,
		hatesTrait ~= nil and hatesTrait:getEffectRange() == ENUM.BonusLimitEffect.noLimit,
		-- the subtype of a hate names a bonus type, and has to come back out as the same key the
		-- hated unit reports for that bonus - which is what makes the hate answerable at all
		hatesTrait ~= nil and hasBonusOfType(other, hatesTrait:getSubtype())
	}

	local report = 1
	for index, passed in ipairs(checks) do
		if not passed then
			report = 1 + index
			break
		end
	end

	server:damageUnit(battle, unit, report)
end

return Script
