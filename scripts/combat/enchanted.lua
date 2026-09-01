local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Keeps a spell permanently applied to its bearer, or to its whole side, by re-applying it
--- every round. Scripted equivalent of the ENCHANTED bonus.
---
--- Parameters:
---  spell    - spell whose effects are applied
---  level    - mastery level the effects are applied at
---  massive  - true to affect every allied unit instead of only the bearer
---  duration - how many turns the effects last

--- Long enough that the effect accumulates rather than expiring between rounds. The effect may
--- still be permanent and last until the end of the battle.
local DEFAULT_DURATION = 50

function Script:targets(battle, unit)
	if not self.massive then
		return { unit }
	end

	return battle:getUnitsIf(function(other)
		return other:getSide() == unit:getSide() and other:isValidTarget(false)
	end)
end

function Script:applyEnchantment(server, battle, unit)
	local spell = LIBRARY:getSpellByName(self.spell)

	if spell == nil then
		error("Unable to apply enchantment - unknown spell " .. tostring(self.spell))
	end

	-- the unit is enchanted by its own ability, so immunities are ignored
	local ignoreImmunity = true

	server:applySpellEffects(battle, unit, spell, self:targets(battle, unit),
		self.level or 0, self.duration or DEFAULT_DURATION, ignoreImmunity)
end

function Script:onBattleStart(server, battle, unit, other)
	self:applyEnchantment(server, battle, unit)
end

function Script:onRoundStart(server, battle, unit, other)
	self:applyEnchantment(server, battle, unit)
end

return Script
