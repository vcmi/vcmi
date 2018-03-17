local ReceiverBase = require("core:erm.ReceiverBase")
local GAME = GAME
local SetResources = require("netpacks.SetResources")
local SERVER = SERVER

local OW = ReceiverBase:new()

function OW:new(ERM)
	return ReceiverBase.new(self,{ERM = ERM})
end

function OW:A(x, ...)
	logError("!!OW:A is not implemented")
end

function OW:C(x, p)
	if p1 == nil then
		return GAME:getCurrentPlayer()
	else
		error("Setting current player in not allowed")
	end
end

function OW:D(x, ...)
	logError("!!OW:D is not implemented")
end

function OW:G(x, ...)
	logError("!!OW:G is not implemented")
end

function OW:H(x, ...)
	logError("!!OW:H is not implemented")
end

function OW:I(x, ...)
	logError("!!OW:I is not implemented")
end

function OW:K(x, ...)
	logError("!!OW:K is not implemented")
end
function OW:N(x, ...)
	logError("!!OW:N is not implemented")
end

function OW:O(x, ...)
	logError("!!OW:O is not implemented")
end

function OW:R(x, player, typ, quantity)
	local p = GAME:getPlayer(player)
	assert(p, "Player ".. tostring(player).." not accessible")

	if quantity == nil then
		return nil, nil, p:getResourceAmount(typ)
	elseif type(quantity) == "table" then
		local hasD = false

		local s = ''

		local v

		for n, k in ipairs(quantity) do
			if k == "d" then
				hasD = true
			else
				v = k
			end
		end

		if not hasD then
			error ("OW:R Special variable expected ".. v .." found")
		end

		if type(v) ~= "number" then
			error ("OW:R Numeric quantity expected".. tostring(v) .." found")
		end

		quantity = v

		local pack = SetResources.new()
		pack:setPlayer(player)
		pack:setAbs(false)
		pack:setAmount(typ, quantity)
		SERVER:commitPackage(pack)
	else
		local pack = SetResources.new()
		pack:setPlayer(player)
		pack:setAbs(true)
		pack:setAmount(typ, quantity)
		SERVER:commitPackage(pack)
	end
end
function OW:S(x, ...)
	logError("!!OW:S is not implemented")
end
function OW:T(x, ...)
	logError("!!OW:T is not implemented")
end
function OW:V(x, ...)
	logError("!!OW:V is not implemented")
end
function OW:W(x, ...)
	logError("!!OW:W is not implemented")
end


return OW
