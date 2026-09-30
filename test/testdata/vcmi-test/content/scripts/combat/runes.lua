local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

local ANIMATIONS = {}
for level = 1, 9 do
	ANIMATIONS[level] = "hota/bulwark/skills/runes/runeLevels/rune" .. level .. "_01.def"
end

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
local YETI = "hota.bulwark:yetiRunemaster"

--- Maximum Yeti rune level; hero skill does not limit it
local YETI_CAP = 9

--- Stat values indexed by rune level
local STATS = {
	{ type = "PRIMARY_SKILL", subtype = "attack",  perLevel = { [0] = 0, 2, 2, 2, 4, 4, 4, 6, 6, 6 } },
	{ type = "PRIMARY_SKILL", subtype = "defence", perLevel = { [0] = 0, 0, 2, 2, 2, 4, 4, 4, 6, 6 } },
	{ type = "STACKS_SPEED",                       perLevel = { [0] = 0, 0, 0, 1, 1, 1, 2, 2, 2, 3 } }
}
local SOUND = "hota/bulwark/spells/RUNE"

--- Only the largest gain source applies per action.
local GAIN_DEFEND = 3
local GAIN_HIT = 2
local GAIN_ATTACK = 1

--- Rune levels pending until ACTION_FINISHED
local PENDING = "RUNE_LEVEL_PENDING"

--- Replaces the counter bonus because its icon refreshes only after re-adding it.
local function setCounter(server, battle, unit, bonusType, value, sourceType, sourceID)
	server:removeUnitBonuses(battle, unit, unit:getBonuses({ type = bonusType }))

	if value > 0 then
		server:addUnitBonus(battle, unit, {
			type       = bonusType,
			sourceType = sourceType or ENUM.BonusSource.other,
			sourceID   = sourceID,
			val        = value,
			valueType  = ENUM.BonusValueType.baseNumber,
			duration   = ENUM.BonusDuration.oneBattle
		}, false)
	end
end

function Script:updateRuneBonuses(server, battle, unit, targetLevel, oldLevel, runeType)
	setCounter(server, battle, unit, runeType.counterType, targetLevel, runeType.sourceType, runeType.sourceID)

	for _, stat in ipairs(STATS) do
		local gained = stat.perLevel[targetLevel] - stat.perLevel[oldLevel]

		if gained > 0 then
			server:addUnitBonus(battle, unit, {
					type       = stat.type,
					subtype    = stat.subtype,
					sourceType = ENUM.BonusSource.other,
					val        = gained,
					valueType  = ENUM.BonusValueType.baseNumber,
					duration   = ENUM.BonusDuration.oneBattle
			}, false)
		end
	end
end

function Script:addRuneLevel(server, battle, unit, oldLevel, amount, runeType, cap)
	local targetLevel = math.min(oldLevel + amount, cap)

	if targetLevel <= oldLevel then
		return oldLevel
	end

	self:updateRuneBonuses(server, battle, unit, targetLevel, oldLevel, runeType)

	return targetLevel
end

--- Returns previous and updated values of the unit's effective rune counter.
function Script:addRuneLevels(server, battle, unit, amount, isYeti)
	local heroLevel = unit:getBonusesValue({ type = RUNE_TYPES.hero.counterType })
	local heroCap = unit:getBonusesValue({ type = "RUNE_LEVEL_CAP" })
	local newHeroLevel = self:addRuneLevel(server, battle, unit, heroLevel, amount, RUNE_TYPES.hero, heroCap)

	if not isYeti then
		return heroLevel, newHeroLevel
	end

	local yetiLevel = unit:getBonusesValue({ type = RUNE_TYPES.yeti.counterType })

	return yetiLevel, self:addRuneLevel(server, battle, unit, yetiLevel, amount, RUNE_TYPES.yeti, YETI_CAP)
end

function Script:processRuneGain(server, battle, unit, amount)
	if not unit:isAlive() then return end

	local oldLevel, newLevel = self:addRuneLevels(server, battle, unit, amount, self.isYeti)

	if oldLevel == newLevel or newLevel == 0 then
		return
	end

	local animation = ANIMATIONS[newLevel]
	if animation then
		server:showBattleAnimation(battle, { { unit = unit } }, animation, SOUND, 1.0, false)
	end

	self:describeGain(server, battle, unit, newLevel)
end

function Script:getPending(unit)
	return unit:getBonusesValue({ type = PENDING })
end

--- Stores the largest gain for the current action.
function Script:record(server, battle, unit, amount)
	if not unit:isAlive() then return end

	if amount > self:getPending(unit) then
		setCounter(server, battle, unit, PENDING, amount)
	end
end

function Script:flush(server, battle, unit)
	local pending = self:getPending(unit)

	if pending == 0 then return end

	setCounter(server, battle, unit, PENDING, 0)
	self:processRuneGain(server, battle, unit, pending)
end

--- Applies starting rune levels to non-siege units and groups animations by resulting level.
function Script:processAltar(server, battle, unit, startLevel)
	local side = unit:getSide()
	local sideUnits = battle:getUnitsIf(function(battleUnit)
		return battleUnit:getSide() == side and battleUnit:getSlot() >= 0
	end)
	local animationTargets = {}

	for _, sideUnit in ipairs(sideUnits) do
		local isYeti = sideUnit:getCreature():getJsonKey() == YETI
		local oldLevel, newLevel = self:addRuneLevels(server, battle, sideUnit, startLevel, isYeti)

		if newLevel ~= oldLevel and newLevel > 0 then
			animationTargets[newLevel] = animationTargets[newLevel] or {}
			table.insert(animationTargets[newLevel], { unit = sideUnit })
		end
	end

	for level = 1, #ANIMATIONS do
		local targets = animationTargets[level]
		if targets then
			server:showBattleAnimation(battle, targets, ANIMATIONS[level], SOUND, 1.0)
		end
	end

	self:describeAltar(server, battle, startLevel)
end

--- Counterattack gain is recorded by onAfterAttacked for the same action.
function Script:onAfterAttack(server, battle, unit, other, payload)
	if payload.isCounter then return end
	self:record(server, battle, unit, GAIN_ATTACK)
end

function Script:onAfterAttacked(server, battle, unit, other, payload)
	self:record(server, battle, unit, GAIN_HIT)
end

function Script:onDefend(server, battle, unit, other)
	self:record(server, battle, unit, GAIN_DEFEND)
end

--- Records damage from hero spells only.
function Script:onSpellHit(server, battle, unit, other, payload)
	if other then return end

	local entry = self:ownEntry(unit, payload)

	if entry and entry.damage > 0 then
		self:record(server, battle, unit, GAIN_HIT)
	end
end

--- Unit spell casts use the attack gain.
function Script:onUnitSpellcast(server, battle, unit, other)
	self:record(server, battle, unit, GAIN_ATTACK)
end

function Script:onActionFinished(server, battle, unit, other)
	self:flush(server, battle, unit)
end

function Script:onBattleStart(server, battle, unit, other)
	local cap = self.isYeti and YETI_CAP or unit:getBonusesValue({ type = "RUNE_LEVEL_CAP" })
	if cap == 0 then return end

	local targetCounterType = self.isYeti and RUNE_TYPES.yeti.counterType or RUNE_TYPES.hero.counterType
	local startLevel = unit:getBonusesValue({ type = "STARTING_RUNE_LEVEL" })
	local currentLevel = unit:getBonusesValue({ type = targetCounterType })

	if math.min(startLevel, cap) > currentLevel then
		self:processAltar(server, battle, unit, startLevel)
	end
end

function Script:describeAltar(server, battle, startLevel)
	if startLevel == 1 then
		server:appendLog(battle, {
			append         = { "core.bonus.RUNE_LEVEL_CAP.description" }
		})
	else
		server:appendLog(battle, {
			append         = { "core.bonus.STARTING_RUNE_LEVEL.description" },
			replaceNumbers = { startLevel }
		})
	end
end

function Script:describeGain(server, battle, unit, newLevel)
	-- Counter type selects the singular or plural log template; both represent the same level.
	local count = unit:getCount()
	server:appendLog(battle, {
		append         = { count == 1 and "core.bonus.RUNE_LEVEL_COUNTER.description" or "core.bonus.YETI_RUNE_LEVEL_COUNTER.description" },
		replaceStrings = { unit:getCreature():getNameTextID(count) },
		replaceNumbers = { newLevel }
	})
end

return Script
