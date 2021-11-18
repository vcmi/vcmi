local TriggerBase = require("core:erm.TriggerBase")
local GameResumed = require("events.GameResumed")

local trigger = TriggerBase:new()

function trigger:new(o)
	o = TriggerBase.new(self, o)

	local id1 = o.id[1]

	if id1 == 0 or id1 == "0" then
		o.sub = GameResumed.subscribeAfter(EVENT_BUS, function(event)
			o:call(event)
		end)
	else
		error ("Identifier "..tostring(id1) .. " not supported by !?GM")
	end
	return o
end

return trigger
