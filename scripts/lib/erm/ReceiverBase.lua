local ReceiverBase = {}

function ReceiverBase:new(o)
	o = o or {}
	setmetatable(o, self)
	self.__index = self
	return o
end

function ReceiverBase:p1Dispatcher(code)
	self[code] = function(self, x, ...)
		local N = select(1, ...)
		return nil, self[code..tostring(N)](self, x, select(2, ...))
	end
end

return ReceiverBase

