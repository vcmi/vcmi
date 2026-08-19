local Script = {}
Script.__index = Script
Script.type = "damageCalculator"

--- Decides what one blow is worth. The engine asks this once per estimate, both for a blow being
--- dealt and for one an AI is only weighing, and takes the whole answer from here - the damage of
--- the creatures, everything that raises or lowers it, and the casualties that follow.
---
--- Every factor is a signed share of the base damage: positive raises it, negative lowers it. What
--- decides how a factor applies is its sign alone, not where it came from, so a mod is free to give
--- any of them the value that flips it. The ones that raise the damage add up; the ones that lower
--- it multiply, each taking its share of what is left.
---
--- A mod extends this through a patch: override `getFactors` and append to what `Base` returned, or
--- override any single step below.

local DAMAGE_TYPE_ALL = "damageTypeAll"
local DAMAGE_TYPE_MELEE = "damageTypeMelee"
local DAMAGE_TYPE_RANGED = "damageTypeRanged"

local MAGIC_ELEMENTAL = "core:magicElemental"
local PSYCHIC_ELEMENTAL = "core:psychicElemental"

--- Integer division as the engine does it, rounding towards zero.
local function idiv(dividend, divisor)
	local quotient = dividend / divisor

	if quotient < 0 then return math.ceil(quotient) end

	return math.floor(quotient)
end

--- The engine's own rounding of a division: not quite to nearest, it takes half a step less.
local function divideAndRound(dividend, divisor)
	if dividend >= 0 then
		return idiv(dividend + idiv(divisor, 2) - 1, divisor)
	end

	return idiv(dividend - idiv(divisor, 2) + 1, divisor)
end

-- Every query starts at the snapshot of bonus types the engine sends along with the attack. Most of
-- what is asked after below is simply not there, and answering that from a table costs nothing;
-- only when the snapshot says a bonus exists is the engine asked what it is worth.

--- Value of every bonus of this type on the unit, combined the way the engine combines them.
local function valueOfType(unit, present, type)
	if not present[type] then return 0 end

	return unit:getBonusesValue({type = type})
end

--- Value of the bonuses of this type that carry the given subtype.
local function valueOfSubtype(unit, present, type, subtype)
	if not present[type] then return 0 end

	return unit:getBonusesValue({type = type, subtype = subtype})
end

local function hasBonusOfType(present, type)
	return present[type] == true
end

--- Value of the bonuses of this type that count in the kind of combat being fought. A bonus limited
--- to melee is simply absent from a shot, and the other way round. Sorted out here rather than by
--- the filter, which knows equality only.
local function valueInThisCombat(unit, present, type, shooting)
	if not present[type] then return 0 end

	return unit:getBonuses({type = type}):filter(function(bonus)
		local range = bonus:getEffectRange()

		if range == ENUM.BonusLimitEffect.noLimit then return true end
		if shooting then return range == ENUM.BonusLimitEffect.onlyDistanceFight end

		return range == ENUM.BonusLimitEffect.onlyMeleeFight
	end):totalValue()
end

-- ---- what the creatures themselves deal ------------------------------------------------------

--- Damage of a single creature, before anything about this particular blow is taken into account.
function Script:getBaseDamageSingle(info)
	local attacker = info.attacker

	if attacker:isTurret() then
		local turretDamage = info.battle:getTurretDamageRange(attacker)

		if #turretDamage == 2 then
			return turretDamage[1], turretDamage[2]
		end
	end

	local minDamage = attacker:getMinDamage(info.shooting)
	local maxDamage = attacker:getMaxDamage(info.shooting)

	if minDamage > maxDamage then
		-- a mod got them the wrong way round; swapping keeps bless and curse meaningful
		minDamage, maxDamage = maxDamage, minDamage
	end

	if hasBonusOfType(info.attackerBonuses, "SIEGE_WEAPON") and not attacker:isTurret() then
		-- a ballista fires for its damage times the attack of the hero owning it
		-- only what the hero itself brings counts, so the two sources are asked after in turn
		local heroAttack = attacker:getBonusesValue({type = "PRIMARY_SKILL", subtype = "attack", sourceType = ENUM.BonusSource.artifact})
			+ attacker:getBonusesValue({type = "PRIMARY_SKILL", subtype = "attack", sourceType = ENUM.BonusSource.heroBaseSkill})

		minDamage = minDamage * (heroAttack + 1)
		maxDamage = maxDamage * (heroAttack + 1)
	end

	return minDamage, maxDamage
end

--- Bless and curse do not scale the damage, they collapse the range onto one of its ends.
function Script:getBaseDamageBlessCurse(info)
	local attacker = info.attacker

	local cursed = hasBonusOfType(info.attackerBonuses, "ALWAYS_MINIMUM_DAMAGE")
	local blessed = hasBonusOfType(info.attackerBonuses, "ALWAYS_MAXIMUM_DAMAGE")

	local shift = valueOfType(attacker, info.attackerBonuses, "ALWAYS_MAXIMUM_DAMAGE")
		- valueOfType(attacker, info.attackerBonuses, "ALWAYS_MINIMUM_DAMAGE")

	local minDamage, maxDamage = self:getBaseDamageSingle(info)

	minDamage = math.max(1, minDamage + shift)
	maxDamage = math.max(1, maxDamage + shift)

	-- both at once is a contradiction no spell of the game can produce; they cancel out
	if cursed and blessed then return minDamage, maxDamage end

	if cursed then return minDamage, minDamage end
	if blessed then return maxDamage, maxDamage end

	return minDamage, maxDamage
end

--- What the whole stack deals, before any factor applies.
function Script:getBaseDamage(info)
	local minDamage, maxDamage = self:getBaseDamageBlessCurse(info)
	local count = info.attacker:getCount()

	return minDamage * count, maxDamage * count
end

-- ---- attack against defense ------------------------------------------------------------------

--- Defense the reducer makes its opponent ignore, as a negative number.
function Script:getDefenseIgnored(info, reducer, present, defense)
	local reduction = valueInThisCombat(reducer, present, "ENEMY_DEFENCE_REDUCTION", info.shooting) / 100

	if reduction <= 0 then return 0 end

	return -math.min(math.floor(reduction * defense) + 1, defense)
end

--- Frenzy trades the defense of its bearer for attack. How much defense there is to trade is
--- decided by the unit being attacked, which is why the conversion happens here.
function Script:getAttackFromFrenzy(info)
	local frenzy = valueOfType(info.attacker, info.attackerBonuses, "IN_FRENZY")

	if frenzy == 0 then return 0 end

	local defense = info.attacker:getDefense(info.shooting)

	return idiv(frenzy * (defense + self:getDefenseIgnored(info, info.defender, info.defenderBonuses, defense)), 100)
end

--- Slayer only reaches a king it is strong enough for.
function Script:getAttackFromSlayer(info)
	if not hasBonusOfType(info.defenderBonuses, "KING") then return 0 end

	local slayer = info.attacker:getBonuses({type = "SLAYER"})

	if slayer:size() == 0 then return 0 end

	local effect = slayer:getBonus(1)
	local mastery = effect:getParametersAsNumber()

	if mastery >= valueOfType(info.defender, info.defenderBonuses, "KING") then return effect:getVal() end

	return 0
end

--- Attack the target makes its attacker lose, as a negative number.
function Script:getAttackIgnored(info, attackBase)
	local reduction = valueInThisCombat(info.defender, info.defenderBonuses, "ENEMY_ATTACK_REDUCTION", info.shooting)

	if reduction <= 0 then return 0 end

	return -math.min(divideAndRound(attackBase * reduction, 100), attackBase)
end

function Script:getAttack(info)
	local base = info.attacker:getAttack(info.shooting)

	return base + self:getAttackFromFrenzy(info) + self:getAttackFromSlayer(info) + self:getAttackIgnored(info, base)
end

function Script:getDefense(info)
	-- a frenzied unit has traded its whole defense away for attack
	if hasBonusOfType(info.defenderBonuses, "IN_FRENZY") then return 0 end

	local base = info.defender:getDefense(info.shooting)

	return base + self:getDefenseIgnored(info, info.attacker, info.attackerBonuses, base)
end

-- ---- the factors -----------------------------------------------------------------------------

--- Attack met with defense. The surplus raises the damage, the shortfall lowers it.
function Script:getAttackDefenseFactor(info)
	local advantage = self:getAttack(info) - self:getDefense(info)

	if advantage > 0 then
		return math.min(info.attackFactorPerPoint * advantage, info.attackFactorCap)
	end

	if advantage < 0 then
		return -math.min(info.defenseFactorPerPoint * -advantage, info.defenseFactorCap)
	end

	return 0
end

--- Offense and archery, whichever of the two this blow is.
function Script:getOffenseArcheryFactor(info)
	local subtype = info.shooting and DAMAGE_TYPE_RANGED or DAMAGE_TYPE_MELEE

	return valueOfSubtype(info.attacker, info.attackerBonuses, "PERCENTAGE_DAMAGE_BOOST", subtype) / 100
end

function Script:getBlessFactor(info)
	return valueOfType(info.attacker, info.attackerBonuses, "GENERAL_DAMAGE_PREMY") / 100
end

function Script:getLuckFactor(info)
	return info.luckyStrike and 1.0 or 0
end

function Script:getJoustingFactor(info)
	if info.chargeDistance <= 0 then return 0 end
	if not hasBonusOfType(info.attackerBonuses, "JOUSTING") then return 0 end
	if hasBonusOfType(info.defenderBonuses, "CHARGE_IMMUNITY") then return 0 end

	return info.chargeDistance * valueOfType(info.attacker, info.attackerBonuses, "JOUSTING") / 100
end

function Script:getFromBackFactor(info)
	local value = valueOfType(info.defender, info.defenderBonuses, "VULNERABLE_FROM_BACK")

	if value == 0 then return 0 end
	if not info.battle:isToReverse(info.attacker, info.defender, info.attackerHex, info.defenderHex) then return 0 end

	return value / 100
end

function Script:getDeathBlowFactor(info)
	return info.deathBlow and 1.0 or 0
end

function Script:getDoubleDamageFactor(info)
	if not info.doubleDamage then return 0 end

	local ownKey = info.attacker:getCreature():getJsonKey()

	return valueOfSubtype(info.attacker, info.attackerBonuses, "BONUS_DAMAGE_PERCENTAGE", ownKey) / 100
end

--- Hate of the creature being struck.
function Script:getHateCreatureFactor(info)
	local hatedKey = info.defender:getCreature():getJsonKey()

	return valueOfSubtype(info.attacker, info.attackerBonuses, "HATE", hatedKey) / 100
end

--- Hate of something the creature being struck happens to be, such as a shooter.
function Script:getHateTraitFactor(info)
	if not hasBonusOfType(info.attackerBonuses, "HATES_TRAIT") then return 0 end

	return info.attacker:getBonuses({type = "HATES_TRAIT"}):filter(function(hate)
		-- the subtype of a hate names a bonus type, which the hated unit is known by carrying
		return hasBonusOfType(info.defenderBonuses, hate:getSubtype())
	end):totalValue() / 100
end

--- A worn-down stack strikes harder, the HotA Haspid ability.
function Script:getRevengeFactor(info)
	if not hasBonusOfType(info.attackerBonuses, "REVENGE") then return 0 end

	local attacker = info.attacker
	local creatureHealth = attacker:getMaxHealth()

	return math.sqrt((attacker:getBaseAmount() + 1) * creatureHealth / (attacker:getAvailableHealth() + creatureHealth) - 1)
end

--- Armorer and everything else that lessens every kind of blow, other than being petrified.
function Script:getArmorerFactor(info)
	if not hasBonusOfType(info.defenderBonuses, "GENERAL_DAMAGE_REDUCTION") then return 0 end

	return info.defender:getBonuses({type = "GENERAL_DAMAGE_REDUCTION", subtype = DAMAGE_TYPE_ALL}):filter(function(bonus)
		return bonus:getSource() ~= ENUM.BonusSource.spellEffect
	end):totalValue() / 100
end

--- Shield and air shield: each lessens one kind of blow and ignores the other.
function Script:getMagicShieldFactor(info)
	local subtype = info.shooting and DAMAGE_TYPE_RANGED or DAMAGE_TYPE_MELEE

	return valueOfSubtype(info.defender, info.defenderBonuses, "GENERAL_DAMAGE_REDUCTION", subtype) / 100
end

--- Shooting too far, or shooting at all with something meant for melee.
function Script:getRangePenaltyFactor(info)
	if info.shooting then
		if info.battle:hasDistancePenalty(info.attacker, info.defender, info.attackerHex, info.defenderHex) then return 0.5 end

		return 0
	end

	if info.attacker:isShooter() and not hasBonusOfType(info.attackerBonuses, "NO_MELEE_PENALTY") then return 0.5 end

	return 0
end

function Script:getObstacleFactor(info)
	if not info.shooting then return 0 end
	if info.battle:hasWallPenalty(info.attacker, info.defender, info.attackerHex, info.defenderHex) then return 0.5 end

	return 0
end

--- Blindness and paralysis, which leave their bearer striking feebly.
function Script:getBlindParalysisFactor(info)
	return valueInThisCombat(info.attacker, info.attackerBonuses, "GENERAL_ATTACK_REDUCTION", info.shooting) / 100
end

function Script:getUnluckyFactor(info)
	return info.unluckyStrike and 0.5 or 0
end

--- Forgetfulness, which only spoils shooting.
function Script:getForgetfulnessFactor(info)
	if not info.shooting then return 0 end

	if not hasBonusOfType(info.attackerBonuses, "FORGETFULL") then return 0 end

	return math.min(valueOfType(info.attacker, info.attackerBonuses, "FORGETFULL"), 100) / 100
end

--- A petrified creature takes half of everything, which is not armour and does not count as it.
function Script:getPetrificationFactor(info)
	if not hasBonusOfType(info.defenderBonuses, "GENERAL_DAMAGE_REDUCTION") then return 0 end

	return info.defender:getBonusesValue({
		type = "GENERAL_DAMAGE_REDUCTION",
		subtype = DAMAGE_TYPE_ALL,
		sourceType = ENUM.BonusSource.spellEffect
	}) / 100
end

--- Magic elementals do half damage to what shrugs off high-level magic.
function Script:getMagicFactor(info)
	if info.attacker:getCreature():getJsonKey() ~= MAGIC_ELEMENTAL then return 0 end
	if valueOfType(info.defender, info.defenderBonuses, "LEVEL_SPELL_IMMUNITY") < 5 then return 0 end

	return 0.5
end

--- Psychic elementals do half damage to what has no mind to attack.
function Script:getMindFactor(info)
	if info.attacker:getCreature():getJsonKey() ~= PSYCHIC_ELEMENTAL then return 0 end
	if not hasBonusOfType(info.defenderBonuses, "MIND_IMMUNITY") then return 0 end

	return 0.5
end

--- Every factor of this blow, signed. A patch appends its own to what this returns.
function Script:getFactors(info)
	return {
		self:getAttackDefenseFactor(info),
		self:getOffenseArcheryFactor(info),
		self:getBlessFactor(info),
		self:getLuckFactor(info),
		self:getJoustingFactor(info),
		self:getFromBackFactor(info),
		self:getDeathBlowFactor(info),
		self:getDoubleDamageFactor(info),
		self:getHateCreatureFactor(info),
		self:getHateTraitFactor(info),
		self:getRevengeFactor(info),
		-self:getArmorerFactor(info),
		-self:getMagicShieldFactor(info),
		-self:getRangePenaltyFactor(info),
		-self:getObstacleFactor(info),
		-self:getBlindParalysisFactor(info),
		-self:getUnluckyFactor(info),
		-self:getForgetfulnessFactor(info),
		-self:getPetrificationFactor(info),
		-self:getMagicFactor(info),
		-self:getMindFactor(info)
	}
end

-- ---- what comes out of it all ----------------------------------------------------------------

--- Most damage the target can take from one blow, whatever the blow is worth.
function Script:getDamageCap(info)
	local percentage = valueOfType(info.defender, info.defenderBonuses, "DAMAGE_RECEIVED_CAP")

	if percentage <= 0 then return math.huge end

	return idiv(info.defender:getMaxHealth() * percentage, 100)
end

--- How many creatures a blow of this size kills.
function Script:getCasualties(info, damage)
	local firstHealth = info.defender:getFirstHPleft()

	if damage < firstHealth then return 0 end

	local killed = 1 + math.floor((damage - firstHealth) / info.defender:getMaxHealth())

	return math.min(killed, info.defender:getCount())
end

function Script:calculate(battle, info)
	-- the battle answers the queries that depend on where the blow happens; it rides along with the
	-- rest of the attack rather than in a global, which a script shared between threads must not have
	info.battle = battle

	local baseMin, baseMax = self:getBaseDamage(info)

	local raising = 1.0
	local lowering = 1.0

	for _, factor in ipairs(self:getFactors(info)) do
		if factor > 0 then
			raising = raising + factor
		elseif factor < 0 then
			-- a factor may take everything, never more
			lowering = lowering * (1 + math.max(-1.0, factor))
		end
	end

	local cap = self:getDamageCap(info)

	local function apply(base, factor)
		return math.min(cap, math.max(1, math.floor(base * factor)))
	end

	local damageMin = apply(baseMin, raising * lowering)
	local damageMax = apply(baseMax, raising * lowering)

	return {
		damage = { min = damageMin, max = damageMax },
		kills = {
			min = self:getCasualties(info, damageMin),
			max = self:getCasualties(info, damageMax)
		},
		-- what the blow would have been worth had the target no defences at all, which is what an
		-- ability reflecting a strike works from
		damageBeforeDefense = { min = apply(baseMin, raising), max = apply(baseMax, raising) }
	}
end

return Script
