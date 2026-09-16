local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Brings its bearer back once per battle, with a share of the size the stack started as rather
--- than of what was left of it. Scripted equivalent of the REBIRTH bonus.
---
--- Parameters:
---  val        - share of the starting size of the stack that comes back, in percent
---  guaranteed - whether at least one creature always comes back, however small the share is

-- effect of the resurrection spell, which the ability has always borrowed its visual from
local ANIMATION = "C01SPE0"
local SOUND     = "RESURECT"

-- own bonus rather than the CASTS the ability used to spend, so that a creature that both
-- rebirths and casts spells does not pay for one out of the other
local SPENT = "REBIRTH_SPENT"

--- Creatures to bring back. The share rarely divides evenly, so the remainder is rolled for:
--- one chance per creature it fell short of, so a small stack comes back some of the time
--- instead of never.
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

--- A resurrected stack cannot retaliate until its next turn.
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
