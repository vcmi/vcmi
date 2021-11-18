local ReceiverBase = require("core:erm.ReceiverBase")

local DO = ReceiverBase:new()

function DO:new(ERM, fun, start, stop, inc)
	assert(fun and start and stop and inc, "!!DO requires 4 arguments")
	return ReceiverBase.new(self,
	{
		ERM=ERM,
		fun=fun,
		start=start,
		stop=stop,
		inc=inc,
	})
end

function DO:P(x, ...)
	local argc = select('#', ...)
	local newx = {}
	local ret = {}

	do
		local __iter, limit, step = tonumber(self.start), tonumber(self.stop), tonumber(self.inc)

		while (step > 0 and __iter <= limit) or (step <= 0 and __iter >= limit) do

			for idx = 1, argc do
				local v = select(idx, ...)

				if v ~= nil then
					newx[tostring(idx)] = v
				end
			end

			newx['16'] = __iter
			self.ERM:callFunction(self.fun, newx)
			__iter = newx['16']
			__iter = __iter + step
		end
	end

	for idx = 1, argc do
		local v = select(idx, ...)
		if v == nil then
			ret[idx] = newx[tostring(idx)]
		else
			ret[idx] = nil
		end
	end

	return unpack(ret)
end


return DO
