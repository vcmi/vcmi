local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Replaces the attacked stack with a stack of another creature, as the WoG werewolf ability does.
--- Scripted equivalent of the TRANSMUTATION bonus.
---
--- Parameters:
---  val         - percentage chance to trigger on each attack
---  creature    - creature the victim turns into. Defaults to the attacker's own creature
---  transmuteBy - "health" keeps the total health of the victim, "count" keeps its creature count

function Script:isImmune(unit)
	return unit:hasBonuses({type = "TRANSMUTATION_IMMUNITY"})
end

function Script:resultingCount(victim, creature)
	if self.transmuteBy == "health" then
		return math.max(math.floor(victim:getCount() * victim:getMaxHealth() / creature:getMaxHealth()), 1)
	end

	if self.transmuteBy == "count" then
		return victim:getCount()
	end

	return nil
end

function Script:onAfterAttack(server, battle, unit, other)
	-- a dead attacker transmutes nothing, and it may have been killed by the retaliation to this attack
	if not unit:isAlive() then return end
	if not other or not other:isAlive() or not other:isLiving() then return end
	if self:isImmune(other) then return end
	if not server:rollCombatAbility(battle, unit, self.val or 0) then return end

	local creature = self.creature and LIBRARY:getCreatureByName(self.creature) or unit:getCreature()

	if other:getCreature():getJsonKey() == creature:getJsonKey() then return end

	local count = self:resultingCount(other, creature)

	if count == nil then return end

	-- removal turns the victim into a ghost with no bonuses and no health, so everything the new
	-- stack inherits is read while it is still a real unit
	local position = other:getPosition()
	local side = other:getSide()

	server:removeUnit(battle, other)
	server:addUnit(battle, {
		count = count,
		type = creature,
		side = side,
		position = position,
		summoned = false
	})
	server:refreshBattleUnits(battle)
end

return Script
