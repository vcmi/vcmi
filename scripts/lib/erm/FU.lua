local ReceiverBase = require("core:erm.ReceiverBase")

local FU = ReceiverBase:new()

function FU:new(ERM, fun)
	assert(fun, "!!FU requires 1 argument")
	return ReceiverBase.new(self,
	{
		ERM=ERM,
		fun=fun,
	})
end


function FU:P(x, ...)
	local argc = select('#', ...)
	local newx = {}
	local ret = {}

	for idx = 1, argc do
		local v = (select(idx, ...))
		idx = tostring(idx)

		if v ~= nil then
			newx[idx] = v
		end
	end

	self.ERM:callFunction(self.fun, newx)

	for idx = 1, argc do
		local v = (select(idx, ...))
		if v == nil then
			ret[idx] = newx[tostring(idx)]
		else
			ret[idx] = nil
		end
	end

	return unpack(ret)
end


return FU
