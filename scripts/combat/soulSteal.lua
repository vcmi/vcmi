local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Raises its bearer's stack for every enemy creature it killed, beyond the stack's original size.
--- Scripted equivalent of the SOUL_STEAL bonus.
---
--- Parameters:
---  val       - creatures gained for each killed enemy creature
---  permanent - true to keep the gained creatures after the battle

-- soul steal has always shared life drain's presentation, since both fed the same heal counter
local ANIMATION = "SP06_"
local SOUND = "DRAINLIF"
local TRANSPARENCY = 0.5

--- Kills among targets whose souls can be taken.
local function stolenSouls(payload)
	local total = 0

	for _, target in ipairs(payload.targets or {}) do
		if target.unit and target.unit:isLiving() then
			total = total + target.killed
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

function Script:onAfterAttack(server, battle, unit, other, payload)
	local gained = stolenSouls(payload) * (self.val or 0)

	if gained <= 0 then return end

	local power = self.permanent and ENUM.HealPower.permanent or ENUM.HealPower.oneBattle
	local healed, resurrected = server:healUnit(battle, unit, gained * unit:getMaxHealth(),
		ENUM.HealLevel.overheal, power)

	if healed <= 0 then return end

	server:showBattleAnimation(battle, { { unit = unit } }, ANIMATION, SOUND, TRANSPARENCY)
	self:describe(server, battle, unit, other, healed, resurrected)
end

return Script
