require("battle.Unit")

local ReceiverBase = require("core:erm.ReceiverBase")

local BU = ReceiverBase:new()

function BU:new(ERM)
	return ReceiverBase.new(self,{ERM = ERM})
end

local BattleLogMessage = require("netpacks.BattleLogMessage")
local BattleUnitsChanged = require("netpacks.BattleUnitsChanged")

local battle = BATTLE

function BU:C(x, p1)
	assert(type(p1) == "nil", "!!BU:C can only check value")

	local ret = battle:isFinished()

	if type(ret) == "nil" then
		return 0
	else
		return 1
	end
end

function BU:D(x, hex, p1)
	assert(type(p1) == "nil", "!!BU:D can only check value")

	local unit = battle:getUnitByPos(hex, false)

	if unit then
		if unit:isAlive() then
			return nil, -2
		else
			return nil, unit:unitId()
		end
	else
		return nil, -1
	end
end

function BU:E(x, hex, p1)
	assert(type(p1) == "nil", "!!BU:E can only check value")

	local unit = battle:getUnitByPos(hex, false)

	if unit and unit:isAlive() then
		return nil, unit:unitId()
	else
		return nil, -1
	end
end

local SPECIAL_FIELDS = {}

SPECIAL_FIELDS['sand_shore'] = 0
SPECIAL_FIELDS['cursed_ground'] = 1
SPECIAL_FIELDS['magic_plains'] = 2
SPECIAL_FIELDS['holy_ground'] = 3
SPECIAL_FIELDS['evil_fog'] = 4
SPECIAL_FIELDS['clover_field'] = 5
SPECIAL_FIELDS['lucid_pools'] = 6
SPECIAL_FIELDS['fiery_fields'] = 7
SPECIAL_FIELDS['rocklands'] = 8
SPECIAL_FIELDS['magic_clouds'] = 9


function BU:G(x, p1)
	assert(type(p1) == "nil", "!!BU:G? is not implemented")

	local bfield = SPECIAL_FIELDS[battle:getBattlefieldType()]

	if bfield then
		return bfield
	else
		return -1
	end
end

function BU:M(x, message)
	local pack = BattleLogMessage.new()
	pack:addText(message)
	SERVER:addToBattleLog(pack)
end

function BU:O(x, ...)
	error("!!BU:O is not implemented")
end

function BU:R(x, ...)
	error("!!BU:R is not implemented")
end

function BU:S(x, typ, count, hex, side, slot)
	local pack = BattleUnitsChanged.new()

	local id = battle:getNextUnitId()

	pack:add(id,
	{
		newUnitInfo =
		{
			["count"] = count,
			["type"] = typ,
			["side"] = side,
			["position"] = hex,
			["summoned"] = (slot == -1),
		}
	})

	SERVER:changeUnits(pack)
end

function BU:T(x)
	local tacticDistance = battle:getTacticDistance()

	if tacticDistance == 0 then
		return 0
	else
		return 1
	end
end

function BU:V(x, ...)
	error("!!BU:V is not implemented")
end

return BU
