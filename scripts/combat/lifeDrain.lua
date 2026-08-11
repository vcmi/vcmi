local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Restores part of the damage its bearer dealt back to it as health, resurrecting fallen
--- creatures of the stack. Scripted equivalent of the LIFE_DRAIN bonus.
---
--- Parameters:
---  val - share of the dealt damage restored to the attacker, in percent

local ANIMATION = "SP06_"
local SOUND = "DRAINLIF"
local TRANSPARENCY = 0.5

--- Damage dealt to targets that life can actually be drained from.
local function drainableDamage(payload)
	local total = 0

	for _, target in ipairs(payload.targets or {}) do
		if target.unit and target.unit:isLiving() then
			total = total + target.damage
		end
	end

	return total
end

function Script:describe(server, battle, unit, other, healed, resurrected)
	-- 361 and 362 are the singular and plural forms of the same message
	local lines = { unit:getCount() == 1 and "core.genrltxt.361" or "core.genrltxt.362" }
	local numbers = { healed }

	if resurrected == 1 then
		table.insert(lines, "core.genrltxt.363")
	elseif resurrected > 1 then
		table.insert(lines, "core.genrltxt.364")
		table.insert(numbers, resurrected)
	end

	local victimName = "core.genrltxt.43" -- "creatures", when the victim is already gone
	if other then
		victimName = other:getCreature():getNameTextID(other:getCount())
	end

	server:appendLog(battle, {
		append         = lines,
		replaceStrings = { unit:getCreature():getNameTextID(unit:getCount()), victimName },
		replaceNumbers = numbers
	})
end

function Script:onAttackResolved(server, battle, unit, other, payload)
	-- a stack at full health has nothing to drain into
	if unit:getTotalHealth() == unit:getAvailableHealth() then return end

	local toHeal = math.floor(drainableDamage(payload) * (self.val or 0) / 100)

	if toHeal <= 0 then return end

	local healed, resurrected = server:healUnit(battle, unit, toHeal,
		ENUM.HealLevel.resurrect, ENUM.HealPower.permanent)

	if healed <= 0 then return end

	server:showBattleAnimation(battle, { { unit = unit } }, ANIMATION, SOUND, TRANSPARENCY)
	self:describe(server, battle, unit, other, healed, resurrected)
end

return Script
