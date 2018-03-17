local TriggerBase = require("core:erm.TriggerBase")
local ApplyDamage = require("events.ApplyDamage")
local eventBus = EVENT_BUS;

local trigger = TriggerBase:new()

function trigger:new(o)
	o = TriggerBase.new(self, o)
	o.sub = ApplyDamage.subscribeBefore(EVENT_BUS, function(event)
		o:call(event)
	end)
	return o
end

return trigger
