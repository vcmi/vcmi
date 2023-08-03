local logError = logError
local bit = bit

local ReceiverBase = require("core:erm.ReceiverBase")
local Metatype = require ("core:Metatype")
local EntitiesChanged = require("netpacks.EntitiesChanged")
local Bonus = require("Bonus")
local BonusBearer = require("BonusBearer")
local BonusList = require("BonusList")

local RES = {[0] = "wood", [1] = "mercury", [2] = "ore", [3] = "sulfur", [4] = "crystal", [5] = "gems", [6] = "gold", [7] = "mithril"}

local SERVICES = SERVICES
local creatures = SERVICES:creatures()
local SERVER = SERVER

local getCreatureByIndex = creatures.getByIndex
local function creatureByIndex(index)
	return getCreatureByIndex(creatures, index)
end

local function sendChanges(creatureIndex, data)
	local pack = EntitiesChanged.new()
	pack:update(Metatype.CREATURE, creatureIndex, data)
	SERVER:commitPackage(pack)
end

local MA = ReceiverBase:new()

function MA:new(ERM)
	return ReceiverBase.new(self,{ERM = ERM})
end

local function checkCreatureIndex(creatureIndex)
	assert(creatureIndex ~= nil, "!!MA requires creature identifier")

	if type(creatureIndex) == "string" then
		error("Identifier resolving is not implemented")
	end

	return creatureIndex
end

local function createModifier(scope, jsonKey, getterKey)

	local f = function (self, x, creatureIndex, p1)
		local creatureIndex = checkCreatureIndex(creatureIndex)

		if p1 == nil then
			local creature = creatureByIndex(creatureIndex)
			return nil, creature[getterKey](creature)
		else
			local packData = {config = {}}

			local config = packData.config

			for _, v in ipairs(scope) do
				config[v] = {}
				config = config[v]
			end

			config[jsonKey] = p1

			sendChanges(creatureIndex, packData)
			return
		end
	end

	return f
end

MA.A = createModifier({}, "attack", "getBaseAttack")
MA.B = createModifier({}, "spellPoints" ,"getBaseSpellPoints")
MA.D = createModifier({}, "defense" ,"getBaseDefense")
MA.E = createModifier({"damage"}, "max", "getBaseDamageMax")
MA.F = createModifier({}, "fightValue" ,"getFightValue")
MA.G = createModifier({}, "growth" ,"getGrowth")
MA.H = createModifier({"advMapAmount"}, "max" ,"getAdvMapAmountMax")
MA.I = createModifier({}, "aiValue" ,"getAIValue")
MA.L = createModifier({}, "level" , "getLevel")
MA.M = createModifier({"damage"}, "min", "getBaseDamageMin")
MA.N = createModifier({}, "shots" , "getBaseShots")
MA.O = createModifier({}, "faction" ,"getFaction")
MA.P = createModifier({}, "hitPoints" ,"getBaseHitPoints")
MA.R = createModifier({}, "horde" , "getHorde")
MA.S = createModifier({}, "speed" , "getBaseSpeed")
MA.V = createModifier({"advMapAmount"}, "min","getAdvMapAmountMin")

function MA:C(x, creatureIndex, resIndex, cost)
	local creatureIndex = checkCreatureIndex(creatureIndex)

	if cost == nil then
		local creature = creatureByIndex(creatureIndex)
		return nil, nil, creature:getCost(resIndex)
	else
		local packData = {config = {cost = {[RES[resIndex]] = cost}}}
		sendChanges(creatureIndex, packData)
	end
end

function MA:U(x, creatureIndex, upgradeIndex)
	-- -2 - no upgrade
	-- -1 - usual upgrade
	logError("!!MA:U is not implemented")
end

local FLAG_NAMES =
{
	--[1] = "doubleWide", --0
	[2] = "FLYING", --1
	[4] = "SHOOTER", --2
	[8] = "TWO_HEX_ATTACK_BREATH", --3
	-- [16] = "alive", --4
	[32] = "CATAPULT", --5
	[64] = "SIEGE_WEAPON", --6
	[128] = "KING1", --7
	[256] = "KING2", --8
	[512] = "KING3", --9
	[1024] = "MIND_IMMUNITY", --10
	--[2048] = "laser shot", --11
	[4096] = "NO_MELEE_PENALTY", --12
	-- [8192] - unused --13
	[16384] = "FIRE_IMMUNITY", --14
	[32768] = "ADDITIONAL_ATTACK", -- val=1 --15
	[65536] = "NO_RETALIATION", --16
	[131072] = "NO_MORALE", --17
	[262144] = "UNDEAD", --18
	[524288] = "ATTACKS_ALL_ADJACENT", --19
	 -- [1048576] - AOE spell-like attack --20
	 -- [2097152] - war machine? --21
	 -- [4194304] = "summoned", --22
	 -- [8388608] = "cloned", --23
}

local FLAGS = {}

for k, v in pairs(FLAG_NAMES) do
	local bonusType = Bonus[v]
	assert(bonusType ~= nil, "Invalid Bonus type: "..v)
	FLAGS[k] = bonusType
end

local FLAGS_REV = {}

for mask, bonusType in pairs(FLAGS) do
	FLAGS_REV[bonusType] = mask
end

function MA:X(x, creatureIndex, flagsMask)
	local creatureIndex = checkCreatureIndex(creatureIndex)
	local creature = creatureByIndex(creatureIndex)
	local creatureBonuses = creature:getBonusBearer()
	local all = creatureBonuses:getBonuses()

	local currentMask = 0

	local toRemove = {}

	do
		local idx = 1
		local bonus = all[idx]

		while bonus do
			local bonusType = bonus:getType()
			local mask = FLAGS_REV[bonusType]

			if mask ~= nil then
				if flagsMask ~= nil then
					if bit.band(mask, flagsMask) == 0 then
						table.insert(toRemove, bonus:toJsonNode())
					end
				end
				currentMask = bit.bor(currentMask, mask)
			end


			idx = idx + 1
			bonus = all[idx]
		end
	end

	if flagsMask == nil then
		local ret = currentMask

		if creature:isDoubleWide() then
			ret = bit.bor(ret, 1)
		end

		return nil, ret
	else
		flagsMask = tonumber(flagsMask)

		local hasChanges = false
		local packData = {config = {}}

		local reqDoubleWide = bit.band(flagsMask, 1) > 0

		if reqDoubleWide ~= creature:isDoubleWide() then
			packData.config.doubleWide, hasChanges = reqDoubleWide, true
		end

		local toAdd = {}

		for mask, name in pairs(FLAG_NAMES) do
			if (bit.band(currentMask, mask) == 0) and (bit.band(flagsMask, mask) ~= 0)  then
				local bonus =
				{
					duration = {[1] = "PERMANENT"},
					source = "CREATURE_ABILITY",
					type = name
				}

				--special case
				if name == "ADDITIONAL_ATTACK" then
					bonus.val = 1
				end

				table.insert(toAdd, bonus)
			end
		end

		hasChanges = hasChanges or #toAdd > 0 or #toRemove > 0

		if hasChanges then
			packData.bonuses = {toAdd = toAdd, toRemove = toRemove}
			sendChanges(creatureIndex, packData)
		end

	end

end

return MA
