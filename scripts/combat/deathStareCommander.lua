local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Patch over the death stare script adding the "commander" situation, where a fixed number of
--- creatures dies instead of every creature of the stack rolling its own chance.
---
--- DEPRECATED, transition only. It exists so that the commander skill converted from the
--- DEATH_STARE bonus keeps working, and reproduces that behaviour rather than generalizing it.
--- A mod that wants a death stare of its own kind should stack a patch like this one over the
--- script instead of asking for another situation here.
---
--- Parameters:
---  val - kills before the level ratio is applied

function Script:killsIn(server, battle, unit, other, payload)
	if self.situation ~= "commander" then
		return Base.killsIn(self, server, battle, unit, other, payload)
	end

	-- worth less against bigger creatures, and nothing at all against those without a level
	local defenderLevel = other:getLevel()

	if defenderLevel <= 0 then return 0 end

	return math.floor(unit:getLevel() * (self.val or 0) / defenderLevel)
end

return Script
