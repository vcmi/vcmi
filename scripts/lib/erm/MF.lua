require("battle.Unit")

local ReceiverBase = require("core:erm.ReceiverBase")

local MF = ReceiverBase:new()

function MF:new(ERM)
	return ReceiverBase.new(self,{ERM = ERM})
end

function MF:D(x)
	return self.ERM.activeEvent:getInitalDamage()
end

function MF:E(x, ...)
	error("!!MF:E is not implemented")
end

function MF:F(x, p1)
	if p1 then
		self.ERM.activeEvent:setDamage(p1)
		return nil
	else
		return self.ERM.activeEvent:getDamage()
	end
end

function MF:N(x)
	return self.ERM.activeEvent:getTarget():unitId()
end

function MF:W(x, ...)
	error("!!MF:W is not implemented")
end

return MF
