local TriggerBase = require("core:erm.TriggerBase")

local trigger = TriggerBase:new()

local eventBus = EVENT_BUS;

local beforeApplyDamage = function(event)
	trigger:call(event)
end

local sub = eventBus:subscribeBefore("ApplyDamage", beforeApplyDamage)

if type(sub) == "string" then
	error("ApplyDamage subscription failed: "..sub)
elseif type(sub) ~= "userdata" then
	error("ApplyDamage subscription failed")
end

trigger.sub1 = sub

return trigger
