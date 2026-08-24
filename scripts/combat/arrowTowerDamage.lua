local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Arrow towers shoot for what their town is worth rather than for what their creature says.
--- The damage is settled once, when the battle is set up, and handed to the tower as a bonus -
--- nothing can change a town's buildings while it is under siege.
---
--- Parameters:
---  keepBase    - damage of the keep in a town with nothing built
---  towerBase   - damage of the two lesser towers in a town with nothing built
---  perBuilding - damage each building adds to the keep; the lesser towers get half of it

local DEFAULTS = { keepBase = 10, towerBase = 6, perBuilding = 2 }

--- Buildings that count towards the damage of the towers. Heroes 3 counts the town hall but not
--- the village hall it replaces, ignores the fort line, and counts a building only once however
--- often it has been upgraded.
--- Asked by building type rather than by json key, so that a mod town's fort counts for as much as
--- the fort of a core town.
local function countsTowardsDamage(building)
	local buildingType = building:getBuildingType()

	if buildingType == "villageHall" or buildingType == "fort" then return false end
	if buildingType == "townHall" then return true end

	return not building:isUpgrade()
end

function Script:getTownLevel(town)
	local level = 0

	for _, building in ipairs(town:getBuildings()) do
		if countsTowardsDamage(building) then level = level + 1 end
	end

	return level
end

--- Lowest and highest damage of one shot of this tower. The highest is twice the lowest, as it is
--- for every creature whose damage Heroes 3 gives as a single number.
function Script:getDamageRange(town, turretPart)
	local level = self:getTownLevel(town)
	local perBuilding = self.perBuilding or DEFAULTS.perBuilding

	local minDamage
	if turretPart == "keep" then
		minDamage = (self.keepBase or DEFAULTS.keepBase) + perBuilding * level
	else
		-- the lesser towers gain half as much, which Heroes 3 rounds down
		minDamage = (self.towerBase or DEFAULTS.towerBase) + perBuilding * math.floor(level / 2)
	end

	return minDamage, minDamage * 2
end

function Script:onBattleSetup(server, battle, unit, other)
	local turretPart = unit:getTurretPart()

	if turretPart == nil then return end

	local town = battle:getDefendedTown()

	-- a tower outside a siege has no town to read; its creature's own damage will do
	if town == nil then return end

	local minDamage, maxDamage = self:getDamageRange(town, turretPart)
	local creatureKey = unit:getCreature():getJsonKey()

	server:addUnitBonus(battle, unit, {
		type = "CREATURE_DAMAGE",
		subtype = "creatureDamageMin",
		val = minDamage,
		duration = "ONE_BATTLE",
		sourceType = "CREATURE_ABILITY",
		sourceID = creatureKey
	}, false)

	server:addUnitBonus(battle, unit, {
		type = "CREATURE_DAMAGE",
		subtype = "creatureDamageMax",
		val = maxDamage,
		duration = "ONE_BATTLE",
		sourceType = "CREATURE_ABILITY",
		sourceID = creatureKey
	}, false)
end

return Script
