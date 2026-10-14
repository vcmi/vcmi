local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Surrounds its bearer with summoned guardians when the battle starts.
--- Scripted equivalent of the SUMMON_GUARDIANS bonus.
---
--- Parameters:
---  creature - creature to summon as guardian
---  val      - size of each guardian stack, in percent of the guarded stack

--- Appends `hex` unless it is off the battlefield or already listed.
local function checkAndPush(hexes, hex)
	if not hex:isAvailable() then return end

	for _, present in ipairs(hexes) do
		if present == hex then return end
	end

	table.insert(hexes, hex)
end

--- Hexes for guarding a unit with double-wide guardians. Covers all hexes surrounding the guarded
--- unit with as few stacks as possible: front, back and one per side for a single-hex target; a
--- wider target needs two per side plus an extra hex in front. Guardians that would land outside
--- the battlefield are dropped, which is what the position checks are for.
function Script:guardianHexes(battle, position, side, targetIsTwoHex)
	local hexes = {}
	local x = position:getX()
	local y = position:getY()
	local fieldWidth = battle:getFieldWidth()
	local targetIsAttacker = side == ENUM.BattleSide.attacker

	-- front guardians. Units starting near the opposite side of the battlefield cannot happen in H3
	if targetIsAttacker then
		checkAndPush(hexes, position:copyToEast():copyToEast())
	else
		checkAndPush(hexes, position:copyToWest():copyToWest())
	end

	if targetIsAttacker and ((y % 2 == 0) or (x > 1)) then
		if targetIsTwoHex and (y % 2 == 1) and (x == 2) then
			checkAndPush(hexes, position:copyToNorthEast())
			checkAndPush(hexes, position:copyToSouthEast())
		else
			-- back-side guardians for a two-hex target, side guardians for a one-hex one
			checkAndPush(hexes, targetIsTwoHex and position:copyToNorthWest() or position:copyToNorthEast())
			checkAndPush(hexes, targetIsTwoHex and position:copyToSouthWest() or position:copyToSouthEast())

			if not targetIsTwoHex and x > 2 then
				checkAndPush(hexes, position:copyToWest())
			elseif targetIsTwoHex then
				checkAndPush(hexes, position:copyToEast():copyToNorthEast())
				checkAndPush(hexes, position:copyToEast():copyToSouthEast())
				if x > 3 then
					checkAndPush(hexes, position:copyToWest():copyToWest())
				end
			end
		end
	elseif not targetIsAttacker and ((y % 2 == 1) or (x < fieldWidth - 2)) then
		if targetIsTwoHex and (y % 2 == 0) and (x == fieldWidth - 3) then
			checkAndPush(hexes, position:copyToNorthWest())
			checkAndPush(hexes, position:copyToSouthWest())
		else
			checkAndPush(hexes, targetIsTwoHex and position:copyToNorthEast() or position:copyToNorthWest())
			checkAndPush(hexes, targetIsTwoHex and position:copyToSouthEast() or position:copyToSouthWest())

			if not targetIsTwoHex and x < fieldWidth - 3 then
				checkAndPush(hexes, position:copyToEast())
			elseif targetIsTwoHex then
				checkAndPush(hexes, position:copyToWest():copyToNorthWest())
				checkAndPush(hexes, position:copyToWest():copyToSouthWest())
				if x < fieldWidth - 4 then
					checkAndPush(hexes, position:copyToEast():copyToEast())
				end
			end
		end
	-- a unit starting against its own edge of the battlefield has no room behind it, so its
	-- guardians go in front instead
	elseif not targetIsAttacker and (y % 2 == 0) then
		checkAndPush(hexes, position:copyToWest():copyToNorthWest())
		checkAndPush(hexes, position:copyToWest():copyToSouthWest())
	elseif targetIsAttacker and (y % 2 == 1) then
		checkAndPush(hexes, position:copyToEast():copyToNorthEast())
		checkAndPush(hexes, position:copyToEast():copyToSouthEast())
	end

	return hexes
end

function Script:onBattleStart(server, battle, unit, other)
	local guardian = LIBRARY:getCreatureByName(self.creature)

	if guardian == nil then
		error("Unable to summon guardians - unknown creature " .. tostring(self.creature))
	end

	local guardianIsBig = guardian:isDoubleWide()
	local side = unit:getSide()
	local hexes

	if guardianIsBig then
		hexes = self:guardianHexes(battle, unit:getPosition(), side, unit:getCreature():isDoubleWide())
	else
		hexes = {}
		local surrounding = unit:getSurroundingHexes()
		for i = 1, surrounding:size() do
			table.insert(hexes, surrounding:at(i))
		end
	end

	local count = math.max(1, math.floor(unit:getCount() * (self.val or 0) / 100))
	local summoned = false

	for _, hex in ipairs(hexes) do
		-- checked per hex, so guardians placed by this loop cannot be overlapped by later ones
		if battle:isAccessibleForNewUnit(hex, guardian, side) then
			server:addUnit(battle, {
				count = count,
				type = guardian,
				side = side,
				position = hex,
				summoned = true
			})
			summoned = true
		end
	end

	if summoned then
		server:refreshBattleUnits(battle)
	end
end

return Script
