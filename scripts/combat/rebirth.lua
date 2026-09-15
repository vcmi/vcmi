local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- A stack that dies comes back with a share of the size it started the battle as, once. The share
--- is a percentage of the starting size rather than of what was left, so a stack worn down over
--- several rounds comes back just as strong as one killed outright.

--- Effect of the resurrection spell, which is what this is meant to look like.
local ANIMATION = "C01SPE0"
local SOUND     = "RESURECT"

--- Marks the rebirth as used up. Its own bonus rather than the CASTS the ability used to spend,
--- so that a creature that both rebirths and casts spells does not pay for one out of the other.
local SPENT = "REBIRTH_SPENT"

--- Creatures to bring back. The share rarely divides evenly, so the remainder is rolled for -
--- one chance per creature the share fell short of, which is what makes a small stack come back
--- some of the time rather than never.
function Script:getRebornCount(server, unit, percentage)
	local baseAmount = unit:getBaseAmount()
	local exact      = baseAmount * percentage / 100
	local count      = math.floor(exact)

	count = count + server:rngBinomial(math.floor(baseAmount - count * 100 / percentage), percentage / 100)

	-- the guaranteed kind never fails to bring back a creature, however small the stack was
	if self.guaranteed then
		count = math.max(count, 1)
	end

	return count
end

--- A stack that is brought back does not get to answer the attack it died to, nor anything else
--- until its own turn comes round again.
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
	-- a clone is a copy that leaves nothing behind, so there is nothing to bring back
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
