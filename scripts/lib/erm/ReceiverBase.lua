local ReceiverBase = {}

function ReceiverBase:new()
	local o = {}
	setmetatable(o, self)
	self.__index = self
	return o
end

return ReceiverBase

