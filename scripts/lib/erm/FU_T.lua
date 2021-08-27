local TriggerBase = require("core:erm.TriggerBase")
local _

local FU = TriggerBase:new()

function FU:call(p1, x)
	self.ERM.activeEvent = nil
	self.ERM.activeTrigger = self
	for _, fn in ipairs(self.fn) do
		fn(self.e, self.y, x)
	end
	self.ERM.activeTrigger = nil
end

return FU
