local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Applies arrow tower damage calculated from defended-town buildings during battle setup.
---
--- Parameters:
---  keepBase    - damage of the keep in a town with nothing built
---  towerBase   - damage of the two lesser towers in a town with nothing built
---  perBuilding - damage each building adds to the keep; the lesser towers get half of it

local DEFAULTS = { keepBase = 10, towerBase = 6, perBuilding = 2 }

--- Implements the H3 building count: town hall instead of village hall, no fort line and only final upgrades.
--- Building types make the rule independent of town mod scope.
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

--- Returns the damage range for one tower shot; H3 doubles the single base value for the maximum.
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

	-- Outside a siege, retain creature damage.
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
