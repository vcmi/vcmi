local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Strikes devoured corpses banked, kept as the value of one bonus rather than as one bonus per
--- strike: bonuses that match each other in every field cannot be removed one at a time.
--- A creature ability of the bearer's own creature is stored the same way, so a devourer that also
--- had a natural double attack would have it counted here - no such creature exists.
local function bankedStrikes(unit)
	local creatureKey = unit:getCreature():getJsonKey()

	return unit:getBonuses({ type = "ADDITIONAL_ATTACK" }):filter(function(bonus)
		return bonus:getSourceID() == creatureKey
	end)
end

function Script:setStrikes(server, battle, unit, count)
	local banked = bankedStrikes(unit)

	if banked:size() > 0 then
		server:removeUnitBonuses(battle, unit, banked)
	end

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

--- Consumes every corpse under the head hex of `unit`, each one worth one extra strike.
function Script:devourCorpses(server, battle, unit)
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

--- Only a move of the unit's own feeds it. A walk-and-attack announces its walk like any other,
--- and does so before the number of blows is settled, so a corpse eaten on the way in is already
--- worth a strike of the attack it walked into.
function Script:onAfterMove(server, battle, unit, other, payload)
	self:devourCorpses(server, battle, unit)
end

--- One banked strike pays for one extra blow. A blow never thrown, because the target died first,
--- costs nothing, which is what keeps unspent strikes for the next attack.
function Script:onAfterAttack(server, battle, unit, other, payload)
	if payload.isCounter then return end
	if payload.attackIndex == 0 then return end

	local banked = bankedStrikes(unit):totalValue()

	if banked > 0 then
		self:setStrikes(server, battle, unit, banked - 1)
	end
end

return Script
