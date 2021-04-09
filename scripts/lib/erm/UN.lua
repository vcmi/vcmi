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

function UN:A(x, artifactIndex, ...)
	error ("UN:A is not implemented")
end

--[[
	WoG chests
]]
function UN:B(x, ...)
	error ("UN:B is not implemented")
end

--[[
	May be reuse for smth
]]
function UN:C(x, ...)
	error ("UN:C is not supported")
end

-- make water passable
function UN:D(x, ...)
	error ("UN:D is not implemented")
end

-- check tile empty
function UN:E(x, ...)
	error ("UN:E is not implemented")
end

-- Fire
function UN:F(x, ...)
	error ("UN:F is not supported")
end


UN:p1Dispatcher("G")

-- Secondary skill texts
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

--[[
	creature ability description, there is a field abilityText, but it`s not used anywhere
]]
function UN:G12(x, creatureIndex, v)
	error ("UN:G1/*/2/* is not supported")
end

-- hero specialty texts
function UN:G2(x, ...)
	error ("UN:G2 is not implemented")
end

-- Set Fog of war (for all but playerIndex)
function UN:H(x, tx, ty, tl, playerIndex, radius)
	error ("UN:H is not supported")
end

-- Put new oblect
function UN:I(x, tx, ty, tl, typ, subtype, ...)
	error ("UN:I is not supported")
end

UN:p1Dispatcher("J")

-- allowed spells
function UN:J0(x, ...)
	error ("UN:J0 is not implemented")
end

-- level XP limits
function UN:J1(x, ...)
	error ("UN:J1 is not implemented")
end

-- difficulty
function UN:J2(x, ...)
	error ("UN:J2 is not implemented")
end

-- WoG settings file
function UN:J3(x, ...)
	error ("UN:J3 is not supported")
end

-- AI think radius
function UN:J4(x, ...)
	error ("UN:J4 is not supported")
end

-- Enable autosave
function UN:J5(x, ...)
	error ("UN:J5 is not implemented")
end

-- generate random artifact
function UN:J6(x, ...)
	error ("UN:J6 is not implemented")
end

-- artifact trader
function UN:J7(x, slot, artifactIndex)
	error ("UN:J7 is not implemented")
end

-- check file exists
function UN:J8(x, ...)
	error ("UN:J8 is not implemented")
end

-- game paths
function UN:J9(x, ...)
	error ("UN:J9 is not supported")
end

-- dump ERM variables
function UN:J10(x, ...)
	error ("UN:J10 is not implemented")
end


function UN:J11(x, ...)
	error ("UN:J11 is not supported")
end

--new week params
function UN:K(x, ...)
	error ("UN:K is not implemented")
end

-- set view point/check obelisk
function UN:L(x, ...)
	error ("UN:L is not implemented")
end

--new month params
function UN:M(x, ...)
	error ("UN:M is not implemented")
end

UN:p1Dispatcher("N")

-- artifact name
function UN:N0(x, z, ...)
	error ("UN:N0 is not implemented")
end

-- spell name
function UN:N1(x, z, index)
	error ("UN:N1 is not implemented")
end

--building name
function UN:N2(x, z, townIndex, index)
	error ("UN:N2 is not implemented")
end

--monster name
function UN:N3(x, z, index)
	error ("UN:N3 is not implemented")
end

--sec skill name
function UN:N4(x, z, index)
	error ("UN:N4 is not implemented")
end

function UN:N5(x, ...)
	error ("UN:N5 is not supported")
end

function UN:N6(x, ...)
	error ("UN:N6 is not supported")
end

--remove object
function UN:O(x, tx, ty, tl)
	error ("UN:O is not implemented")
end

-- WoG options
function UN:P(x, ...)
	error ("UN:P is not implemented")
end

-- end game for current player
function UN:Q(x, ...)
	error ("UN:Q is not implemented")
end

function UN:R(x, ...)
	error ("UN:R is not supported")
end

-- Remove Fog of war for all playerIndex)
function UN:H(x, tx, ty, tl, playerIndex, radius)
	error ("UN:H is not supported")
end

--set available creatures
function UN:T(x, town, level, dwellingSlot, creature)
	error ("UN:T is not implemented")
end

-- count objects, get coordiantes
function UN:U(x, ...)
	error ("UN:U is not implemented")
end

-- version info
function UN:V(x, ...)
	error ("UN:V is not implemented")
end

-- make water passable
function UN:W(x, ...)
	error ("UN:W is not implemented")
end

-- map size
function UN:X(x, ...)
	error ("UN:X is not implemented")
end

return UN
