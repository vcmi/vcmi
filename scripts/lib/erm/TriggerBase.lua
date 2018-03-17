local TriggerBase = {}

function TriggerBase:new()
	local o =
	{
		fn = {},
		--y = {}
		--e = {}
	}

	setmetatable(o, self)
	self.__index = self

	return o
end

function TriggerBase:call(event)
	self.ERM.activeEvent = event
	self.ERM.activeTrigger = self
	for _, fn in ipairs(self.fn) do
		fn()
	end
	self.ERM.activeTrigger = nil
	self.ERM.activeEvent = nil
end


return TriggerBase
