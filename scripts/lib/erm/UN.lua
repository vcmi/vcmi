local ReceiverBase = require("core:erm.ReceiverBase")
local Metatype = require ("core:Metatype")
local EntitiesChanged = require("netpacks.EntitiesChanged")

local SERVICES = SERVICES
local creatures = SERVICES:creatures()
local SERVER = SERVER


local function sendChanges(metatype, index, data)
	local pack = EntitiesChanged.new()
	pack:update(metatype, index, data)
	SERVER:commitPackage(pack)
end

local UN = ReceiverBase:new()

function UN:new(ERM)
	return ReceiverBase.new(self,{ERM=ERM})
end

UN:p1Dispatcher("G")

function UN:G0(x, ...)
	error ("UN:G0 is not implemented")
end

function UN:G1(x, creatureIndex, N, v)
	return nil, nil, (self["G1"..tostring(N)](self, x, creatureIndex, v))
end

function UN:G10(x, creatureIndex, v)
	if v == nil then
		return creatures:getByIndex(creatureIndex):getSingularName()
	else
		v = self.ERM:getZVar(v)
		local packData = {config = {name={singular=v}}}
		sendChanges(Metatype.CREATURE, creatureIndex, packData)
	end
end
function UN:G11(x, creatureIndex, v)
	if v == nil then
		return creatures:getByIndex(creatureIndex):getPluralName()
	else
		v = self.ERM:getZVar(v)
		local packData = {config = {name={plural=v}}}
		sendChanges(Metatype.CREATURE, creatureIndex, packData)
	end
end
function UN:G12(x, creatureIndex, v)
	error ("UN:G1/*/2/* is not implemented")
end

function UN:G2(x, ...)
	error ("UN:G2 is not implemented")
end

return UN
