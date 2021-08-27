local TriggerBase = {}

function TriggerBase:new(o)
	o = o or {}
	o.id = o.id or {}
	o.fn = o.fn or {}
	o.y = o.y or {}
	o.e = o.e or {}

	setmetatable(o, self)
	self.__index = self
	return o
end

function TriggerBase:call(event)
	self.ERM.activeEvent = event
	self.ERM.activeTrigger = self
	for _, fn in ipairs(self.fn) do
		fn(self.e, self.y)
	end
	self.ERM.activeTrigger = nil
	self.ERM.activeEvent = nil
end

function TriggerBase:addHandler(fn)
	table.insert(self.fn, fn)
end

function TriggerBase:checkSub(sub, sub_name)
	if type(sub) == "string" then
		error(sub_name .. " subscription failed: "..sub)
	elseif type(sub) ~= "userdata" then
		error(sub_name .. " subscription failed")
	end
end

return TriggerBase
