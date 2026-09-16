local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

local ANIMATIONS = {
	"hota/bulwark/skills/runes/runeLevels/rune1_01.def",
	"hota/bulwark/skills/runes/runeLevels/rune2_01.def",
	"hota/bulwark/skills/runes/runeLevels/rune3_01.def",
	"hota/bulwark/skills/runes/runeLevels/rune4_01.def",
	"hota/bulwark/skills/runes/runeLevels/rune5_01.def",
	"hota/bulwark/skills/runes/runeLevels/rune6_01.def",
	"hota/bulwark/skills/runes/runeLevels/rune7_01.def",
	"hota/bulwark/skills/runes/runeLevels/rune8_01.def",
	"hota/bulwark/skills/runes/runeLevels/rune9_01.def"
}
local RUNE_TYPES = {
	hero = {
		counterType = "RUNE_LEVEL_COUNTER",
		sourceType = ENUM.BonusSource.secondarySkill,
		sourceID = "runes"
	},
	yeti = {
		counterType = "YETI_RUNE_LEVEL_COUNTER",
		sourceType = ENUM.BonusSource.creatureAbility,
		sourceID = "yetiRunemaster"
	}
}
local ATTACK_BONUS = {
	[0] = 0,
	[1] = 2,
	[2] = 2,
	[3] = 2,
	[4] = 4,
	[5] = 4,
	[6] = 4,
	[7] = 6,
	[8] = 6,
	[9] = 6
}
local DEFENSE_BONUS = {
	[0] = 0,
	[1] = 0,
	[2] = 2,
	[3] = 2,
	[4] = 2,
	[5] = 4,
	[6] = 4,
	[7] = 4,
	[8] = 6,
	[9] = 6
}
local SPEED_BONUS = {
	[0] = 0,
	[1] = 0,
	[2] = 0,
	[3] = 1,
	[4] = 1,
	[5] = 1,
	[6] = 2,
	[7] = 2,
	[8] = 2,
	[9] = 3
}
local SOUND = "hota/bulwark/spells/RUNE"

--- What one action is worth. Only the largest of these applies, so a unit that strikes and is
--- struck in return gains the 2 of the blow it took rather than 3 for having done both.
local GAIN_DEFEND = 3
local GAIN_HIT = 2
local GAIN_ATTACK = 1

--- Levels an action earned but that are not granted yet.
local PENDING = "RUNE_LEVEL_PENDING"

--- Returns both hero-granted and Yeti-ability granted current rune levels
function Script:getCurrentRuneLevels(unit)
	return unit:getBonusesValue({ type = RUNE_TYPES.hero.counterType }),
	       unit:getBonusesValue({ type = RUNE_TYPES.yeti.counterType })
end

--- Updates the current rune level bonuses from oldLevel to targetLevel
function Script:updateRuneBonuses(server, battle, unit, targetLevel, oldLevel, runeType)
	--- remove and re-add the counter to keep the icon up-to-date
	local runeLevelBonuses = unit:getBonuses({ type = runeType.counterType })
	server:removeUnitBonuses(battle, unit, runeLevelBonuses)
	server:addUnitBonus(battle, unit, {
			type       = runeType.counterType,
			sourceType = runeType.sourceType,
			sourceID   = runeType.sourceID,
			val        = targetLevel,
			valueType  = ENUM.BonusValueType.baseNumber,
			duration   = ENUM.BonusDuration.oneBattle
	}, false)

	local bonusVal = ATTACK_BONUS[targetLevel] - ATTACK_BONUS[oldLevel]
	if bonusVal > 0 then
		server:addUnitBonus(battle, unit, {
				type       = "PRIMARY_SKILL",
				subtype    = "attack",
				sourceType = ENUM.BonusSource.other,
				val        = bonusVal,
				valueType  = ENUM.BonusValueType.baseNumber,
				duration   = ENUM.BonusDuration.oneBattle
		}, false)
	end
	bonusVal = DEFENSE_BONUS[targetLevel] - DEFENSE_BONUS[oldLevel]
	if bonusVal > 0 then
		server:addUnitBonus(battle, unit, {
				type       = "PRIMARY_SKILL",
				subtype    = "defence",
				sourceType = ENUM.BonusSource.other,
				val        = bonusVal,
				valueType  = ENUM.BonusValueType.baseNumber,
				duration   = ENUM.BonusDuration.oneBattle
		}, false)
	end
	bonusVal = SPEED_BONUS[targetLevel] - SPEED_BONUS[oldLevel]
	if bonusVal > 0 then
		server:addUnitBonus(battle, unit, {
				type       = "STACKS_SPEED",
				sourceType = ENUM.BonusSource.other,
				val        = bonusVal,
				valueType  = ENUM.BonusValueType.baseNumber,
				duration   = ENUM.BonusDuration.oneBattle
		}, false)
	end
end

--- Adds both types of rune levels, any positive amount, up to the cap
function Script:addRuneLevel(server, battle, unit, oldLevel, amount, runeType, cap)
	if cap == 0 then
		return 0
	end

	local targetLevel = math.min(oldLevel + amount, cap)

	if targetLevel == oldLevel then
		return oldLevel
	end

	self:updateRuneBonuses(server, battle, unit, targetLevel, oldLevel, runeType)

	return targetLevel
end

--- Adds hero-granted rune levels
function Script:addHeroRuneLevels(server, battle, unit, oldLevel, amount)
	local cap = unit:getBonusesValue({ type = "RUNE_LEVEL_CAP" })

	return self:addRuneLevel(server, battle, unit, oldLevel, amount, RUNE_TYPES.hero, cap)
end

--- Adds Yeti-ability rune levels
function Script:addYetiRuneLevels(server, battle, unit, oldLevel, amount)
	return self:addRuneLevel(server, battle, unit, oldLevel, amount, RUNE_TYPES.yeti, 9)
end

--- Attempts to add "amount" of rune levels, both hero-granted and Yeti-ability granted ones.
--- If any rune level was successfully added, it plays the animation of the reached rune level.
function Script:processRuneGain(server, battle, unit, amount, deferAnimation)
	if not unit:isAlive() then return end
	local isYeti = self.isYeti
	local heroRuneLevel, yetiRuneLevel = self:getCurrentRuneLevels(unit)
	local oldLevel = isYeti and yetiRuneLevel or heroRuneLevel

	heroRuneLevel = self:addHeroRuneLevels(server, battle, unit, heroRuneLevel, amount)
	if isYeti then
		yetiRuneLevel = self:addYetiRuneLevels(server, battle, unit, yetiRuneLevel, amount)
	end

	local newLevel = isYeti and yetiRuneLevel or heroRuneLevel

	if oldLevel == newLevel or newLevel == 0 then
		return
	end

	local animation = ANIMATIONS[newLevel]
	if animation then
		server:showBattleAnimation(battle, { { unit = unit } }, animation, SOUND, 1.0, deferAnimation)
	end

	self:describe(server, battle, unit, newLevel)
end

--- Levels the current action has earned so far.
function Script:getPending(unit)
	return unit:getBonusesValue({ type = PENDING })
end

function Script:setPending(server, battle, unit, value)
	local pending = unit:getBonuses({ type = PENDING })

	if pending:size() > 0 then
		server:removeUnitBonuses(battle, unit, pending)
	end

	if value > 0 then
		server:addUnitBonus(battle, unit, {
			type       = PENDING,
			sourceType = ENUM.BonusSource.other,
			val        = value,
			valueType  = ENUM.BonusValueType.baseNumber,
			duration   = ENUM.BonusDuration.oneBattle
		}, false)
	end
end

--- Banks what the running action is worth so far. Only its largest single reason counts.
function Script:record(server, battle, unit, amount)
	if not unit:isAlive() then return end

	if amount > self:getPending(unit) then
		self:setPending(server, battle, unit, amount)
	end
end

--- Grants what the finished action was worth.
function Script:flush(server, battle, unit)
	local pending = self:getPending(unit)

	if pending == 0 then return end

	self:setPending(server, battle, unit, 0)
	self:processRuneGain(server, battle, unit, pending, false)
end

--- Batch-processes the starting rune levels (both types) and adds them to every non-siege unit on the caller's side.
--- Plays an animation on all units that changed any rune level. Animations are played in ascending order of rune level acquired.
function Script:processAltar(server, battle, unit, startLevel)
	local side = unit:getSide()
	local sideUnits = battle:getUnitsIf(function(battleUnit)
		return battleUnit:getSide() == side and battleUnit:getSlot() >= 0
	end)
	local animationTargets = {}

	for _, sideUnit in ipairs(sideUnits) do
		local heroRuneLevel, yetiRuneLevel = self:getCurrentRuneLevels(sideUnit)
		local isYeti = sideUnit:getCreature():getJsonKey() == "hota.bulwark:yetiRunemaster"
		local oldLevel = isYeti and yetiRuneLevel or heroRuneLevel
		heroRuneLevel = self:addHeroRuneLevels(server, battle, sideUnit, heroRuneLevel, startLevel)
		if isYeti then
			yetiRuneLevel = self:addYetiRuneLevels(server, battle, sideUnit, yetiRuneLevel, startLevel)
		end

		local newLevel = isYeti and yetiRuneLevel or heroRuneLevel
		if newLevel ~= oldLevel and newLevel > 0 then
			animationTargets[newLevel] = animationTargets[newLevel] or {}
			table.insert(animationTargets[newLevel], { unit = sideUnit })
		end
	end

	for level = 1, 9 do
		local targets = animationTargets[level]
		if targets then
			server:showBattleAnimation(battle, targets, ANIMATIONS[level], SOUND, 1.0)
		end
	end

	self:describe(server, battle, nil, startLevel)
end

--- Called after `unit` attacked `other`. A counterattack belongs to the attacker's action, and the
--- unit that made it is credited for the blow it took instead.
function Script:onAfterAttack(server, battle, unit, other, payload)
	if payload.isCounter then return end
	self:record(server, battle, unit, GAIN_ATTACK)
end

--- Called after `unit` was attacked by `other`.
function Script:onAfterAttacked(server, battle, unit, other, payload)
	self:record(server, battle, unit, GAIN_HIT)
end

--- Called when `unit` defends.
function Script:onDefend(server, battle, unit, other)
	self:record(server, battle, unit, GAIN_DEFEND)
end

--- Called on a unit a spell reached. Only a hero's spell earns anything, and one cast is one action
--- however many units it damaged.
function Script:onSpellHit(server, battle, unit, other, payload)
	if other then return end

	local entry = self:ownEntry(unit, payload)

	if entry and entry.damage > 0 then
		self:record(server, battle, unit, GAIN_HIT)
	end
end

--- Called when `unit` casts a spell, which spends its turn and so counts as attacking.
function Script:onUnitSpellcast(server, battle, unit, other)
	self:record(server, battle, unit, GAIN_ATTACK)
end

--- Called once the action that reached this unit is over, counterattack and repeated blows
--- included. What the whole of it was worth is granted here, rather than blow by blow.
function Script:onActionFinished(server, battle, unit, other)
	self:flush(server, battle, unit)
end

--- Called once for every unit present when the battle starts, after tactics are over.
function Script:onBattleStart(server, battle, unit, other)
	local cap = self.isYeti and 9 or unit:getBonusesValue({ type = "RUNE_LEVEL_CAP" })
	if cap == 0 then return end

	local targetCounterType = self.isYeti and RUNE_TYPES.yeti.counterType or RUNE_TYPES.hero.counterType
	local startLevel = unit:getBonusesValue({ type = "STARTING_RUNE_LEVEL" })
	local currentLevel = unit:getBonusesValue({ type = targetCounterType })

	if math.min(startLevel, cap) > currentLevel then
		self:processAltar(server, battle, unit, startLevel)
	end
end

--- Dispatches battle log descriptions.
function Script:describe(server, battle, unit, newLevel)
	if not unit then
		if newLevel == 1 then
			server:appendLog(battle, {
				append         = { "core.bonus.RUNE_LEVEL_CAP.description" }
			})
		else
			server:appendLog(battle, {
				append         = { "core.bonus.STARTING_RUNE_LEVEL.description" },
				replaceNumbers = { newLevel }
			})
		end
	else
		-- the two counter bonuses double as the singular and plural wording of this line - which
		-- of the two rune levels was gained does not change what the log says
		local count = unit:getCount()
		server:appendLog(battle, {
			append         = { count == 1 and "core.bonus.RUNE_LEVEL_COUNTER.description" or "core.bonus.YETI_RUNE_LEVEL_COUNTER.description" },
			replaceStrings = { unit:getCreature():getNameTextID(count) },
			replaceNumbers = { newLevel }
		})
	end
end

return Script
