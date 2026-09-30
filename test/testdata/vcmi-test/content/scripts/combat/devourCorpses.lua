local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Returns banked strikes stored in the creature's ADDITIONAL_ATTACK bonus.
--- Matching bonuses cannot be removed individually.
local function bankedStrikes(unit)
	local creatureKey = unit:getCreature():getJsonKey()

	return unit:getBonuses({ type = "ADDITIONAL_ATTACK" }):filter(function(bonus)
		return bonus:getSourceID() == creatureKey
	end)
end

function Script:setStrikes(server, battle, unit, count)
	server:removeUnitBonuses(battle, unit, bankedStrikes(unit))

	if count > 0 then
		server:addUnitBonus(battle, unit, {
			duration   = ENUM.BonusDuration.oneBattle,
			type       = "ADDITIONAL_ATTACK",
			sourceType = ENUM.BonusSource.creatureAbility,
			sourceID   = unit:getCreature():getJsonKey(),
			val        = count
		}, false)
	end
end

--- Consumes corpses under the head hex and grants one strike per corpse.
--- Movement before an attack executes before attack count calculation.
function Script:onAfterMove(server, battle, unit, other, payload)
	if not unit:isAlive() then return end

	local headHex = unit:getPosition()
	local corpses = battle:getUnitsIf(function(target)
		return target:isDead() and not target:isGhost() and target:coversPos(headHex)
	end)

	if #corpses == 0 then
		return
	end

	for _, corpse in ipairs(corpses) do
		server:removeUnit(battle, corpse)
	end

	self:setStrikes(server, battle, unit, bankedStrikes(unit):totalValue() + #corpses)
	server:refreshBattleUnits(battle)
end

--- Consumes one banked strike for each executed additional attack.
function Script:onAfterAttack(server, battle, unit, other, payload)
	if payload.isCounter then return end
	if payload.attackIndex == 0 then return end

	local banked = bankedStrikes(unit):totalValue()

	if banked > 0 then
		self:setStrikes(server, battle, unit, banked - 1)
	end
end

return Script
