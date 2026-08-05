local Base = require("combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Test fixture: damages whoever attacks the protected unit.
--- Only onAfterAttacked is overridden - every other combat event falls through
--- to the no-op inherited from combatScript.
function Script:onAfterAttacked(server, battle, unit, other)
	if not other or not other:isAlive() then return end

	server:damageUnit(battle, other, self.damage or self.casterPower or 0)
end

return Script
