local Base = require("spells/unitEffect")
local BattleLog = require("battleLog")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Returns the pattern for the given direction, measured from one hex of the caster.
--- Can return invalid hexes and hexes of the caster itself, so needs to be checked later.
local function getPatternFromDirection(casterPos, direction)
	local hexes = {}
	if direction == 0 then
		local first = casterPos:copyToEast()
		local second = casterPos:copyToNorthEast()
		local last = casterPos:copyToSouthEast()
		hexes[1] = first
		hexes[2] = second
		hexes[3] = last
		hexes[4] = first:copyToEast()
		hexes[5] = first:copyToNorthEast()
		hexes[6] = first:copyToSouthEast()
		hexes[7] = second:copyToNorthEast()
		hexes[8] = last:copyToSouthEast()
		return hexes
	elseif direction == 1 then
		local first = casterPos:copyToEast()
		local second = first:copyToSouthWest()
		local last = second:copyToWest()
		hexes[1] = first
		hexes[2] = second
		hexes[3] = last
		hexes[4] = first:copyToEast()
		hexes[5] = first:copyToSouthEast()
		hexes[6] = second:copyToSouthEast()
		hexes[7] = second:copyToSouthWest()
		hexes[8] = last:copyToSouthWest()
		return hexes
	elseif direction == 2 then
		local first = casterPos:copyToSouthWest()
		local second = first:copyToEast()
		local last = first:copyToWest()
		hexes[1] = first
		hexes[2] = second
		hexes[3] = last
		hexes[4] = first:copyToSouthEast()
		hexes[5] = first:copyToSouthWest()
		hexes[6] = second:copyToSouthEast()
		hexes[7] = last:copyToSouthWest()
		return hexes
	elseif direction == 3 then
		local first = casterPos:copyToSouthEast()
		local second = casterPos:copyToSouthWest()
		local last = casterPos:copyToWest()
		hexes[1] = first
		hexes[2] = second
		hexes[3] = last
		hexes[4] = first:copyToSouthWest()
		hexes[5] = first:copyToSouthEast()
		hexes[6] = second:copyToWest()
		hexes[7] = second:copyToSouthWest()
		hexes[8] = last:copyToWest()
		return hexes
	elseif direction == 4 then
		local first = casterPos:copyToWest()
		local second = first:copyToNorthEast()
		local last = first:copyToSouthEast()
		hexes[1] = first
		hexes[2] = second
		hexes[3] = last
		hexes[4] = first:copyToWest()
		hexes[5] = first:copyToNorthWest()
		hexes[6] = first:copyToSouthWest()
		hexes[7] = second:copyToNorthWest()
		hexes[8] = last:copyToSouthWest()
		return hexes
	elseif direction == 5 then
		local first = casterPos:copyToNorthWest()
		local second = casterPos:copyToNorthEast()
		local last = casterPos:copyToWest()
		hexes[1] = first
		hexes[2] = second
		hexes[3] = last
		hexes[4] = first:copyToWest()
		hexes[5] = first:copyToNorthEast()
		hexes[6] = first:copyToNorthWest()
		hexes[7] = second:copyToNorthEast()
		hexes[8] = last:copyToWest()
		return hexes
	elseif direction == 6 then
		local first = casterPos:copyToNorthWest()
		local second = casterPos:copyToNorthEast()
		local last = first:copyToWest()
		hexes[1] = first
		hexes[2] = second
		hexes[3] = last
		hexes[4] = first:copyToNorthWest()
		hexes[5] = first:copyToNorthEast()
		hexes[6] = second:copyToNorthEast()
		hexes[7] = last:copyToNorthWest()
		return hexes
	elseif direction == 7 then
		local first = casterPos:copyToNorthEast()
		local second = casterPos:copyToNorthWest()
		local last = casterPos:copyToEast()
		hexes[1] = first
		hexes[2] = second
		hexes[3] = last
		hexes[4] = first:copyToNorthWest()
		hexes[5] = first:copyToNorthEast()
		hexes[6] = first:copyToEast()
		hexes[7] = second:copyToNorthWest()
		hexes[8] = last:copyToEast()
		return hexes
	end

	return hexes
end

--- Thank you, ChatGPT! I left all its comments in as a tribute to its service.
local function getHexDirection(castX, castY, destX, destY)
	local dx = destX - castX
	local dy = destY - castY

	-- Even rows are shifted right
	if castY % 2 == 0 then
		dx = dx - 0.5
	end

	if destY % 2 == 0 then
		dx = dx + 0.5
	end

	local angle = math.deg(math.atan2(dy, dx))
	if angle < 0 then
		angle = angle + 360
	end

	-- Convert to 8 directions
	return math.floor((angle + 22.5) / 45) % 8
end

--- The half of a double-wide caster the cone is measured from: the western one when it strikes
--- west, the eastern one otherwise. Which hex that is depends on the side the unit fights on - the
--- second hex of a double-wide unit lies west of it for the attacker and east of it for the
--- defender - so the footprint is asked rather than assumed.
local function getAnchor(caster, direction)
	local hexes = caster:getHexes()
	local anchor = hexes:at(1)
	local wantWest = direction >= 3 and direction <= 5

	for i = 2, hexes:size() do
		local other = hexes:at(i)
		if (wantWest and other:getX() < anchor:getX()) or (not wantWest and other:getX() > anchor:getX()) then
			anchor = other
		end
	end

	return anchor
end

--- Convenience function for getPatternFromDirection
local function getAffectedHexes(mechanics, spellTarget)
	if #spellTarget == 0 then return {} end
	local aim = spellTarget[1].hex
	local caster = mechanics:getUnitCaster()
	local head = caster:getPosition()
	local direction = getHexDirection(head:getX(), head:getY(), aim:getX(), aim:getY())
	local pattern = {}

	for _, hex in ipairs(getPatternFromDirection(getAnchor(caster, direction), direction)) do
		if hex:isValid() and not caster:coversPos(hex) then
			table.insert(pattern, hex)
		end
	end

	return pattern
end

--- Returns true if the strike is lucky or unlucky
local function rollForLuck(server, luckDice)
	if luckDice <= 0 then
		return false
	end

	return server:rngInt(1, 24) <= luckDice
end

--- Cannot target itself, dead units, units specifically immune to Heat Stroke and cannot target an invincible unit
function Script:isEligible(mechanics, unit, target, targetID)
	if unit:unitID() == targetID or not target:isValidTarget(false) or target:isInvincible() then
		return false
	end

	return mechanics:isReceptive(target)
end

--- Here, targets[1] needs to contain the spellTarget hex (which is the one we are aiming at)
--- It is used to lock casting to the cursor being inside the pattern hexes
function Script:transformTarget(mechanics, aimPoint, spellTarget)
	local battle = mechanics:getBattle()
	local casterUnit = mechanics:getUnitCaster()
	local pattern = getAffectedHexes(mechanics, spellTarget)
	local targets = {}
	local seenUnits = {}

	table.insert(targets, { unit = nil, hex = spellTarget[1].hex })

	for _, hex in ipairs(pattern) do
		if hex:isValid() then
			local target = battle:getUnitByPos(hex, true)

			if target then
				local id = target:unitID()
				if not seenUnits[id] then
					seenUnits[id] = true
					if self:isEligible(mechanics, casterUnit, target, id) then
						table.insert(targets, { unit = target, hex = hex })
					end
				end
			end
		end
	end

	return targets
end

--- Affected hexes are the pattern of the direction, nothing else
function Script:adjustAffectedHexes(mechanics, hexes, spellTarget)
	local pattern = getAffectedHexes(mechanics, spellTarget)
	for _, h in ipairs(pattern) do
		if h:isValid() then
			hexes:insert(h)
		end
	end
	return hexes
end

--- Unit-only spell
function Script:applicableGeneral(mechanics, problem)
	local caster = mechanics:getUnitCaster()
	if not caster or not caster:isAlive() then
		problem:addGeneric(mechanics)
		return false
	end
	return true
end

--- No need to check target units here, just lock casting into the pattern hexes
function Script:applicableTarget(mechanics, problem, target)
	local pattern = getAffectedHexes(mechanics, target)
	local targetHex = target[1].hex

	if not targetHex then
		problem:addStandard(mechanics, ENUM.SpellCastProblem.wrongSpellTarget)
		return false
	end

	for _, hex in ipairs(pattern) do
		if targetHex == hex then
			return true
		end
	end

	problem:addStandard(mechanics, ENUM.SpellCastProblem.wrongSpellTarget)
	return false
end

function Script:apply(mechanics, server, target)
	local battle = mechanics:getBattle()
	local caster = mechanics:getUnitCaster()
	local count = caster:getCount()
	local baseDamMin = caster:getMinDamage(false) * count
	local baseMaxDam = caster:getMaxDamage(false) * count
	local attack = caster:getAttack(false)
	local totalDamage, totalKilled = 0, 0
	--- asked of the engine rather than summed, so that the cap and the units luck does not reach
	--- are answered for us
	local luck = caster:getLuck()
	local isUnluck = luck < 0
	--- bad luck is rolled on twice the dice good luck is
	local luckDice = isUnluck and -luck * 2 or luck

	--- TODO: actual damage calculation (probably needs engine support for accuracy)
	for _, dest in ipairs(target) do
		local unit = dest.unit
		if unit then
			local damage = server:rngInt(baseDamMin, baseMaxDam) --- note: not quite correct mechanics-wise, but fine for temporary
			local defense = unit:getDefense(false)
			local add = attack - defense
			local factor = 0
			if add > 0 then
				factor = math.min(1 + add * 0.05, 4.0)
			else
				factor = math.max(1 + add * 0.025, 0.3)
			end
			damage = damage * factor
			if rollForLuck(server, luckDice) then
				damage = isUnluck and math.floor(damage * 0.5) or math.floor(damage * 2)
			end
			local cap = unit:getBonusesValue({ type = "DAMAGE_RECEIVED_CAP" })
			if cap > 0 then
				damage = math.max(math.floor(unit:getMaxHealth() * cap / 100), 1)
			end
			local dealt, killed = server:damageUnit(battle, unit, damage)
			totalDamage = totalDamage + dealt
			totalKilled = totalKilled + killed
		end
	end
	local victim = #target == 2 and target[2].unit or nil

	--- looked up here rather than once at the top of the script: the services a script sees while
	--- its context is being built are not the ones a battle runs on
	BattleLog.spellDamage(server, battle, LIBRARY:getSpellByName("heatStroke"), victim, totalDamage, totalKilled)
end

return Script
