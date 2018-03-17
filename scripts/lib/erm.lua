DATA = DATA or {}
local DATA = DATA
local GAME = GAME

local TurnStarted = require("events.TurnStarted")

DATA.ERM = DATA.ERM or {}

local ERM_D = DATA.ERM

ERM_D.F = ERM_D.F or {}
ERM_D.MDATA = ERM_D.MDATA or {}
ERM_D.Q = ERM_D.Q or {}
ERM_D.v = ERM_D.v or {}
ERM_D.z = ERM_D.z or {}

local INT_MT =
{
	__index = function(t, key)
		return rawget(t, key) or 0
	end
}

local STR_MT =
{
	__index = function(t, key)
		return rawget(t, key) or ""
	end
}

setmetatable(DATA.ERM.Q, INT_MT)
setmetatable(DATA.ERM.v, INT_MT)
setmetatable(DATA.ERM.z, STR_MT)

local ERM =
{
	w = {},
	M = {},
	subs = {}
}

local ERM_MT =
{
	__index = DATA.ERM
}

setmetatable(ERM, ERM_MT)

local MACROS_MT =
{
	__index = function(M, key)
		address = rawget(ERM.MDATA, key)
		assert(address, "Macro "..key.." is not defined")
		return ERM[address.name][address.index]
	end,
	__newindex = function(M, key, value)
		address = rawget(ERM.MDATA, key)
		assert(address, "Macro "..key.." is not defined")
		ERM[address.name][address.index] = value
	end
}

setmetatable(ERM.M, MACROS_MT)

function ERM:addMacro(name, varName, varIndex)
	assert(varName == "v" or varName == "z", "Macro for "..varName.. "[...] is not allowed")
	rawset(self.MDATA, name, {name = varName, index = varIndex})
end

function ERM:getZVar(p1)
	if type(p1) == "number" then
		return self.z[tostring(p1)]
	else
		return p1
	end
end

local HERO_VAR_MT =
{
	__index = function(t, key)
		ERM_D.WDATA = ERM_D.WDATA or {}
		local wKey = ERM_D.wKey

		if not wKey then
			error("Hero variable set is not selected")
		end

		ERM_D.WDATA[wKey] = ERM_D.WDATA[wKey] or {}

		return ERM_D.WDATA[wKey][key] or 0
	end,
	__newindex = function(t, key, value)
		ERM_D.WDATA = ERM_D.WDATA or {}
		local wKey = ERM_D.wKey

		if not wKey then
			error("Hero variable set is not selected")
		end

		ERM_D.WDATA[wKey] = ERM_D.WDATA[wKey] or {}
		ERM_D.WDATA[wKey][key] = value
	end
}

setmetatable(ERM.w, HERO_VAR_MT)

local function dailyUpdate(event)
	ERM.Q.c = GAME:getDate(0)
end

table.insert(ERM.subs, TurnStarted.subscribeAfter(EVENT_BUS, dailyUpdate))

local Receivers = {}

local function createReceiverLoader(name)
	local loader = function(...)
		Receivers[name] = Receivers[name] or require("core:erm."..name)
		return Receivers[name]:new(ERM, ...)
	end
	return loader
end

--[[

AR
BA
BF
BG
BH
CA
CD
CE
CM
DL
CO
EA
EX
GE
HL
HO
HT
LE
MO
MR
MW
OB
PM
PO
QW
SS
TL
TR

]]

local supportedReceivers = {"BM", "BU", "DO", "FU", "HE", "IF", "DO", "MA", "MF", "OW", "TM", "UN", "VR"}

for _, v in pairs(supportedReceivers) do ERM[v] = createReceiverLoader(v) end

local Triggers = {}

local function createTriggerLoader(name)
	local loader = function(id_list, id_key)

		local t = Triggers[id_key]

		if not t then
			local p = require("core:erm."..name.."_T")
			t = p:new{ERM=ERM, id=id_list}
			Triggers[id_key] = t
		end

		return t
	end
	return loader
end

--[[
!?AE
!?BA
!?BF
!?BG
!?BR
!?CM client only?
!?CO
!?DL client only?
!?GE
!?HE
!?HL
!?HM
!?LE (!$LE)
!?MG
!?MM client only?
!?MR
!?MW
!?OB (!$OB)
!?SN client only?
!?SN (ERA)
!?TH client only?
!?TL client only? depends on time limit feature
]]

local supportedTriggers = {"PI", "FU", "GM", "MF", "TM"}

local TriggerLoaders = {}

for _, v in pairs(supportedTriggers) do TriggerLoaders[v] = createTriggerLoader(v) end

local function makeIdKey(name, id_list)
	if id_list == nil then
		id_list = {}
	elseif type(id_list) ~= "table" then
		id_list = {id_list}
	end

	return name .. table.concat(id_list, "|")
end

function ERM:addTrigger(t)
	local name = t.name
	local fn = t.fn
	local id_list = t.id or {}

	local id_key = makeIdKey(name, id_list)

	local trigger = TriggerLoaders[name](id_list, id_key)

	trigger:addHandler(fn)
end

function ERM:callFunction(id, x)
	local id_key = makeIdKey("FU", id)

	local t = Triggers[id_key]
	if t then
		t:call({}, x)
	end
end

function ERM:callInstructions(cb)
	if not DATA.ERM.instructionsCalled then
		cb()
		local id_key = makeIdKey("PI")
		local pi = Triggers[id_key]
		if pi then
			pi:call()
		end
		DATA.ERM.instructionsCalled = true
	end
end

return ERM
