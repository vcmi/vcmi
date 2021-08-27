local TriggerBase = require("core:erm.TriggerBase")
local ObjectVisitStarted = require("events.ObjectVisitStarted")
local eventBus = EVENT_BUS
local game = GAME

local trigger = TriggerBase:new()

function trigger:new(o)
	o = TriggerBase.new(self, o)

	local id1 = tonumber(o.id[1])

	o.sub = ObjectVisitStarted.subscribeBefore(eventBus,
	function(event)
		local objIndex = event:getObject()

		local obj = game:getObj(objIndex, false)

		if obj:getObjGroupIndex() == id1 then
			o:call(event)
		end
	end)

	return o
end

return trigger
