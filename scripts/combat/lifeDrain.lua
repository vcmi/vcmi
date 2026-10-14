local Base = require("combat/combatScript")
local BattleLog = require("battleLog")
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

function Script:onAfterAttack(server, battle, unit, other, payload)
	-- a stack at full health has nothing to drain into
	if unit:getTotalHealth() == unit:getAvailableHealth() then return end

	local toHeal = math.floor(drainableDamage(payload) * (self.val or 0) / 100)

	if toHeal <= 0 then return end

	-- the log names the stack as it was before it drained, which resurrecting may grow
	local drainerCount = unit:getCount()

	local healed, resurrected = server:healUnit(battle, unit, toHeal,
		ENUM.HealLevel.resurrect, ENUM.HealPower.permanent)

	if healed <= 0 then return end

	server:showBattleAnimation(battle, { { unit = unit } }, ANIMATION, SOUND, TRANSPARENCY)
	BattleLog.lifeDrained(server, battle, unit, other, healed, resurrected, drainerCount)
end

return Script
