--local TriggerBase = require("core:erm.TriggerBase")
--local ReceiverBase = require("core:erm.ReceiverBase")

DATA = DATA or {}

local ERM = {}

local DATA = DATA

DATA.ERM = DATA.ERM or {}

local ERM_MT =
{
	__index = DATA.ERM
}

setmetatable(ERM, ERM_MT)

DATA.ERM.flag = DATA.ERM.flag or {}
DATA.ERM.quick = DATA.ERM.quick or {}
DATA.ERM.v = DATA.ERM.v or {}
DATA.ERM.z = DATA.ERM.z or {}

--TriggerBase.ERM = ERM
--ReceiverBase.ERM = ERM

local y = {}

ERM.getY = function(key)
	y[key] = y[key] or {}
	return y[key]
end

local Receivers = {}

local function createReceiverLoader(name)
	local loader = function(x, ...)
		Receivers[name] = Receivers[name] or require("core:erm."..name)
		local o = Receivers[name]:new(x, ...)
		o.ERM = ERM
		return o
	end
	return loader
end

ERM.BM = createReceiverLoader("BM")
ERM.BU = createReceiverLoader("BU")
ERM.IF = createReceiverLoader("IF")
ERM.MF = createReceiverLoader("MF")

local Triggers = {}

local function createTriggerLoader(name)
	local loader = function(...)
		Triggers[name] = Triggers[name] or require("core:erm."..name.."_T")
		Triggers[name].ERM = ERM
		return Triggers[name]
	end
	return loader
end

local TriggerLoaders = {}

TriggerLoaders.PI = createTriggerLoader("PI")
TriggerLoaders.MF = createTriggerLoader("MF")


function ERM:addTrigger(t)
	local name = t.name
	local fn = t.fn

	local trigger = TriggerLoaders[name]()

	table.insert(trigger.fn, fn)
end

function ERM:callInstructions(cb)
	if not DATA.ERM.instructionsCalled then
		cb()
		self:callTrigger("PI")
		DATA.ERM.instructionsCalled = true
	end
end

function ERM:callTrigger(name)
	local trigger = TriggerLoaders[name]()
	trigger:call()
end

return ERM
