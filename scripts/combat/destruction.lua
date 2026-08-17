local Base = require("combat/combatScript")
local BattleLog = require("battleLog")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Kills creatures of the attacked stack outright, on top of the damage the attack itself dealt.
--- Scripted equivalent of the DESTRUCTION bonus.
---
--- Parameters:
---  val    - percentage chance to trigger on each attack
---  killBy - "percentage" kills a share of the victim's stack, "count" kills a fixed number
---  amount - the share, or the number of creatures, depending on killBy

-- animation and sound of the slayer spell, which the ability has always borrowed its visual from
local ANIMATION = "C13SPW0"
local SOUND = "SLAYER"

function Script:creaturesToKill(victim)
	if self.killBy == "percentage" then
		return math.floor(victim:getCount() * (self.amount or 0) / 100)
	end

	if self.killBy == "count" then
		return self.amount or 0
	end

	return 0
end

function Script:onAfterAttack(server, battle, unit, other)
	-- a dead attacker destroys nothing, and it may have been killed by the retaliation to this attack
	if not unit:isAlive() then return end
	if not other or not other:isAlive() then return end
	if not server:rollCombatAbility(battle, unit, self.val or 0) then return end

	local toKill = self:creaturesToKill(other)

	if toKill <= 0 then return end

	-- deferred so that the spell effect and the death animation start on the same frame
	server:showBattleAnimation(battle, { { unit = other } }, ANIMATION, SOUND, 1.0, true)

	-- the top creature of the stack may already be wounded, so this can kill one more than asked
	local _, killed = server:damageUnit(battle, other, toKill * other:getMaxHealth())

	BattleLog.creaturesPerish(server, battle, other, killed)
end

return Script
