local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Resurrects a percentage of the initial stack count once per battle.
---
--- Parameters:
---  val        - percentage of the initial stack count to resurrect
---  guaranteed - whether to resurrect at least one creature

-- Use the resurrection spell audiovisual effect.
local ANIMATION = "C01SPE0"
local SOUND     = "RESURECT"

-- A separate marker prevents rebirth from consuming creature spell casts.
local SPENT = "REBIRTH_SPENT"

--- Returns the randomly rounded percentage of the initial stack count.
function Script:getRebornCount(server, unit, percentage)
	local baseAmount = unit:getBaseAmount()
	local exact      = baseAmount * percentage / 100
	local count      = math.floor(exact)

	count = count + server:rngBinomial(math.floor(baseAmount - count * 100 / percentage), percentage / 100)

	-- Guaranteed rebirth restores at least one creature.
	if self.guaranteed then
		count = math.max(count, 1)
	end

	return count
end

--- Prevents retaliation until the next turn.
function Script:spendAnswer(server, battle, unit)
	server:addUnitBonus(battle, unit, {
		type       = "NO_RETALIATION",
		val        = 0,
		duration   = ENUM.BonusDuration.nTurns,
		turns      = 1,
		sourceType = ENUM.BonusSource.creatureAbility,
		sourceID   = unit:getCreature():getJsonKey()
	}, false)
end

function Script:onDeath(server, battle, unit, other, payload)
	-- Clones have no corpse to resurrect.
	if unit:isClone() then return end
	if unit:hasBonuses({ type = SPENT }) then return end

	local percentage = self.val or 0

	if percentage <= 0 then return end

	local count = self:getRebornCount(server, unit, percentage)

	if count <= 0 then return end

	server:addUnitBonus(battle, unit, {
		type       = SPENT,
		val        = 1,
		duration   = ENUM.BonusDuration.oneBattle,
		sourceType = ENUM.BonusSource.creatureAbility,
		sourceID   = unit:getCreature():getJsonKey()
	}, false)

	server:showBattleAnimation(battle, { { unit = unit } }, ANIMATION, SOUND, 0.5, true)
	server:healUnit(battle, unit, count * unit:getMaxHealth(), ENUM.HealLevel.resurrect, ENUM.HealPower.permanent)

	self:spendAnswer(server, battle, unit)
end

return Script
