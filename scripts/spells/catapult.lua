local Base = require("spells/spellEffect")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

local WALLS  = { ENUM.WallPart.upperWall, ENUM.WallPart.overGate,
                 ENUM.WallPart.belowGate, ENUM.WallPart.bottomWall }
local TOWERS = { ENUM.WallPart.upperTower, ENUM.WallPart.bottomTower }
local KEEP   = ENUM.WallPart.keep
local GATE   = ENUM.WallPart.gate

--- Position of every target along the town wall, from its top to its bottom. Gate stands
--- between two central wall segments, keep and towers next to the segment they belong to
local POSITION = {
	[ENUM.WallPart.upperTower]  = 0,
	[ENUM.WallPart.upperWall]   = 1,
	[ENUM.WallPart.overGate]    = 2,
	[ENUM.WallPart.gate]        = 3,
	[ENUM.WallPart.belowGate]   = 4,
	[ENUM.WallPart.bottomWall]  = 5,
	[ENUM.WallPart.keep]        = 6,
	[ENUM.WallPart.bottomTower] = 6,
}

local function pickRandom(server, list)
	if #list == 0 then return nil end
	return list[server:rngInt(1, #list)]
end

--- Returns entry with the lowest value, picking at random between equally valued entries
local function pickLowest(server, list, value)
	local lowest, lowestValue = {}, nil
	for _, entry in ipairs(list) do
		local entryValue = value(entry)
		if lowestValue == nil or entryValue < lowestValue then
			lowest, lowestValue = { entry }, entryValue
		elseif entryValue == lowestValue then
			lowest[#lowest+1] = entry
		end
	end
	return pickRandom(server, lowest)
end

--- Returns all parts from the specified list that can still be attacked
local function attackable(battle, parts)
	local list = {}
	for _, part in ipairs(parts) do
		if battle:isWallPartAttackable(part) then list[#list+1] = part end
	end
	return list
end

--- Returns every target that catapult may attack in this town
local function potentialTargets(battle)
	local parts = { GATE, KEEP }
	for _, part in ipairs(WALLS) do parts[#parts+1] = part end
	for _, part in ipairs(TOWERS) do parts[#parts+1] = part end
	return attackable(battle, parts)
end

--- Returns target closest to the specified one, picking at random between equally close targets
local function nearestTarget(server, from, candidates)
	return pickLowest(server, candidates, function(part) return math.abs(POSITION[part] - POSITION[from]) end)
end

--- Returns wall segment with the least hitpoints left, picking at random between equally damaged ones
local function mostDamagedWall(battle, server)
	return pickLowest(server, attackable(battle, WALLS), function(part) return battle:getWallState(part) end)
end

--- Shot that has missed its target hits nearest wall segment that is still standing.
--- If all wall segments are destroyed then such shot hits its intended target instead
local function redirectShot(battle, server, desired)
	local walls = {}
	for _, part in ipairs(attackable(battle, WALLS)) do
		if part ~= desired then walls[#walls+1] = part end
	end

	if #walls == 0 then return desired end
	return nearestTarget(server, desired, walls)
end

--- Returns target for a catapult that aims on its own - when hero has no Ballistics,
--- when catapult is set to automatic, and during quick combat
local function automaticTarget(battle, server, hasBallistics)
	if hasBallistics then
		-- Catapult of a hero with Ballistics brings down the drawbridge first and keeps
		-- shooting at walls only while all of wall segments are still standing
		if battle:isWallPartAttackable(GATE) then return GATE end
		if #attackable(battle, WALLS) == #WALLS then return mostDamagedWall(battle, server) end
		if battle:isWallPartAttackable(KEEP) then return KEEP end

		return pickRandom(server, attackable(battle, TOWERS)) or mostDamagedWall(battle, server)
	end

	-- Catapult without Ballistics turns to other targets only once all walls are destroyed
	local wallTarget = mostDamagedWall(battle, server)
	if wallTarget ~= nil then return wallTarget end
	if battle:isWallPartAttackable(GATE) then return GATE end
	if battle:isWallPartAttackable(KEEP) then return KEEP end

	return pickRandom(server, attackable(battle, TOWERS))
end

function Script:hitChance(part)
	if part == GATE then return self.chanceToHitGate or 0 end
	if part == KEEP then return self.chanceToHitKeep or 0 end
	if part == ENUM.WallPart.bottomTower
	   or part == ENUM.WallPart.upperTower then return self.chanceToHitTower or 0 end
	return self.chanceToHitWall or 0
end

function Script:randomDamage(server)
	local crit  = math.min(100, math.max(0, self.chanceToCrit       or 0))
	local hit   = math.min(100 - crit, math.max(0, self.chanceToNormalHit or 0))
	local noDmg = 100 - hit - crit
	local chances = { noDmg, hit, crit }
	local r, acc = server:rngInt(0, 99), 0
	for dmg = 0, 2 do
		acc = acc + chances[dmg + 1]
		if r < acc then return dmg end
	end
	return 0
end

--- Earthquake deals its damage one point at a time, assigning each point to a random target.
--- This intentionally differs from H3, where selection of a target is clearly buggy: keep and
--- towers are counted as targets even in a town that has not built them, and a point may be
--- assigned to a target that is already scheduled for destruction, which wastes it.
--- As result, in H3 Earthquake deals less damage than it should, especially in towns without Castle
function Script:applyEarthquake(battle, server, attacker, points)
	local targets = potentialTargets(battle)
	local damagePerPart, order = {}, {}

	for _ = 1, points do
		local candidates = {}
		for _, part in ipairs(targets) do
			if (damagePerPart[part] or 0) < battle:getWallState(part) then candidates[#candidates+1] = part end
		end

		local part = pickRandom(server, candidates)
		if part == nil then break end

		if not damagePerPart[part] then
			order[#order+1] = part
			damagePerPart[part] = 0
		end
		damagePerPart[part] = damagePerPart[part] + self:randomDamage(server)
	end

	-- All points are assigned before any damage is applied
	for _, part in ipairs(order) do
		server:catapultAttack(battle, attacker, part, damagePerPart[part])
	end
end

function Script:applicableGeneral(mechanics, problem)
	local battle = mechanics:getBattle()
	if not battle:hasFortifications()
	   or (mechanics:isSmart() and mechanics:getCasterSide() ~= ENUM.BattleSide.attacker)
	   or #potentialTargets(battle) == 0 then
		problem:addStandard(mechanics, ENUM.SpellCastProblem.noAppropriateTarget)
		return false
	end
	return true
end

function Script:apply(mechanics, server, target)
	local battle    = mechanics:getBattle()
	local attacker  = mechanics:getUnitCaster()
	local shots     = self.targetsToAttack or 0

	if mechanics:isMassive() then
		self:applyEarthquake(battle, server, attacker, shots)
		return
	end

	local desired = target[1] and battle:hexToWallPart(target[1].hex) or ENUM.WallPart.invalid

	if not battle:isWallPartAttackable(desired) then
		desired = automaticTarget(battle, server, mechanics:getEffectLevel() > 0)
	end

	for _ = 1, shots do
		if desired == nil then return end

		if not battle:isWallPartAttackable(desired) then
			-- Target was destroyed by previous shot, so remaining shots are aimed
			-- at the nearest target of any type that is still standing
			desired = nearestTarget(server, desired, potentialTargets(battle))
			if desired == nil then return end
		end

		local actual = desired
		if server:rngInt(0, 99) >= self:hitChance(desired) then
			actual = redirectShot(battle, server, desired)
		end

		server:catapultAttack(battle, attacker, actual, self:randomDamage(server))
	end
end

return Script
