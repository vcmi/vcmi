local Base = require("combat/combatScript")
local BattleLog = require("battleLog")
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

function Script:onAfterAttack(server, battle, unit, other, payload)
	local gained = stolenSouls(payload) * (self.val or 0)

	if gained <= 0 then return end

	local power = self.permanent and ENUM.HealPower.permanent or ENUM.HealPower.oneBattle
	-- the log names the stack as it was before it grew
	local stealerCount = unit:getCount()
	local healed, resurrected = server:healUnit(battle, unit, gained * unit:getMaxHealth(),
		ENUM.HealLevel.overheal, power)

	if healed <= 0 then return end

	server:showBattleAnimation(battle, { { unit = unit } }, ANIMATION, SOUND, TRANSPARENCY)
	BattleLog.lifeDrained(server, battle, unit, other, healed, resurrected, stealerCount)
end

return Script
