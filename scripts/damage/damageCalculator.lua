local Script = {}
Script.__index = Script
Script.type = "damageCalculator"

--- Script that computes total damage dealt by one unit to another during combat

--- Core logic is: game computes base damage, applies all damage modifiers (factors) to it, and computes resulting damage range

--- A mod extends this through a patch: write a factor of its own and hand it to `addDamageFactor`, or override any single step below.

local DAMAGE_TYPE_ALL = "damageTypeAll"
local DAMAGE_TYPE_MELEE = "damageTypeMelee"
local DAMAGE_TYPE_RANGED = "damageTypeRanged"

--- Integer division as the engine does it, rounding towards zero.
local function idiv(dividend, divisor)
	local quotient = dividend / divisor

	if quotient < 0 then return math.ceil(quotient) end

	return math.floor(quotient)
end

--- Optimization - list of bonuses that engine will check for presence before passing them to Lua
--- Allows quick estimation of whether damage factor is active in the first place
Script.declaredBonuses = {}

--- All known damage factors, calculated on script initialization
Script.damageFactors = {}

--- Allows script patch to request additional bonus for engine to report
--- NOTE: must be done in script body, not in function. See built-in patches for examples
function Script:declareBonus(type)
	self.declaredBonuses[type] = true
end

--- Adds another damage modifier factor to attack, by the name of the method computing it.
function Script:addDamageFactor(method)
	if self[method] == nil then
		error("damage calculator is given a factor " .. method .. ", which is not one of its methods")
	end

	for _, name in ipairs(self.damageFactors) do
		if name == method then
			error("damage calculator is given the factor " .. method .. " twice")
		end
	end

	table.insert(self.damageFactors, method)
end

--- Function that engine uses to collect list of requested bonuses 
function Script:bonusTypes()
	local types = {}

	for type in pairs(self.declaredBonuses) do
		table.insert(types, type)
	end

	return types
end

--- Optimization support - quickly check whether bonus is present on unit
--- Can only be used on bonuses declared via `declareBonus` call
local function hasBonusOfType(present, type)
	-- a patch declares into this very table, so what a patch added is seen here too
	if not Script.declaredBonuses[type] then
		error("damage calculator asks after bonus " .. type .. ", which it never declared")
	end

	return present[type] == true
end

--- Value of every bonus of this type on the unit, combined the way the engine combines them.
local function getBonusValueOfType(unit, present, type)
	if not hasBonusOfType(present, type) then return 0 end

	return unit:getBonusesValue({type = type})
end

--- Value of the bonuses of this type that carry the given subtype.
local function getBonusValueOfSubtype(unit, present, type, subtype)
	if not hasBonusOfType(present, type) then return 0 end

	return unit:getBonusesValue({type = type, subtype = subtype})
end

--- Value of the bonuses of this type that count in the kind of combat being fought. A bonus limited
--- to melee is simply absent from a shot, and the other way round.
local function getBonusValueOfTypeAndRange(unit, present, type, shooting)
	if not hasBonusOfType(present, type) then return 0 end

	return unit:getBonusesValue({type = type, shooting = shooting})
end

-- The same four, as methods - a patch is a chunk of its own and cannot see the locals above. The
-- base script keeps calling the locals, which spares it a walk up the whole chain of patches.

--- The engine's own rounding of a division: not quite to nearest, it takes half a step less.
--- Here for patches that reproduce a rule the engine rounds this way.
function Script:divideAndRound(dividend, divisor)
	if dividend >= 0 then
		return idiv(dividend + idiv(divisor, 2) - 1, divisor)
	end

	return idiv(dividend - idiv(divisor, 2) + 1, divisor)
end

--- Whether the unit carries this bonus. Reading the table directly does the same, but goes unnoticed
--- when the type was never declared - this says so instead.
function Script:hasBonusOfType(present, type)
	return hasBonusOfType(present, type)
end

--- Value of every bonus of this type the unit carries. Answers 0 without asking the engine when the
--- unit has none, which is the usual case.
function Script:getBonusValueOfType(unit, present, type)
	return getBonusValueOfType(unit, present, type)
end

--- Value of the bonuses of this type that carry the given subtype.
function Script:getBonusValueOfSubtype(unit, present, type, subtype)
	return getBonusValueOfSubtype(unit, present, type, subtype)
end

--- Value of the bonuses of this type that count in the kind of combat being fought.
function Script:getBonusValueOfTypeAndRange(unit, present, type, shooting)
	return getBonusValueOfTypeAndRange(unit, present, type, shooting)
end

-- ---- what the creatures themselves deal ------------------------------------------------------

--- Damage of a single creature, before anything about this particular blow is taken into account.
function Script:getBaseDamageSingle(info)
	local attacker = info.attacker

	local minDamage = attacker:getMinDamage(info.shooting)
	local maxDamage = attacker:getMaxDamage(info.shooting)

	if minDamage > maxDamage then
		-- the config of a creature is caught when it loads, so what reaches here is a bonus that
		-- turned the range around
		error("creature " .. attacker:getCreature():getJsonKey() .. " has minimal damage ("
			.. minDamage .. ") greater than maximal damage (" .. maxDamage .. ")")
	end

	return minDamage, maxDamage
end

--- Bless and curse do not scale the damage, they collapse the range onto one of its ends.
function Script:getBaseDamageBlessCurse(info)
	local attacker = info.attacker

	local cursed = hasBonusOfType(info.attackerBonuses, "ALWAYS_MINIMUM_DAMAGE")
	local blessed = hasBonusOfType(info.attackerBonuses, "ALWAYS_MAXIMUM_DAMAGE")

	local shift = getBonusValueOfType(attacker, info.attackerBonuses, "ALWAYS_MAXIMUM_DAMAGE")
		- getBonusValueOfType(attacker, info.attackerBonuses, "ALWAYS_MINIMUM_DAMAGE")

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
	local reduction = getBonusValueOfTypeAndRange(reducer, present, "ENEMY_DEFENCE_REDUCTION", info.shooting) / 100

	if reduction <= 0 then return 0 end

	return -math.min(math.floor(reduction * defense) + 1, defense)
end

--- Frenzy trades the defense of its bearer for attack. How much defense there is to trade is
--- decided by the unit being attacked, which is why the conversion happens here.
function Script:getAttackFromFrenzy(info)
	local frenzy = getBonusValueOfType(info.attacker, info.attackerBonuses, "IN_FRENZY")

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

	if mastery >= getBonusValueOfType(info.defender, info.defenderBonuses, "KING") then return effect:getVal() end

	return 0
end

--- Attack the target makes its attacker lose, as a negative number. Nothing of the game does this -
--- the rule lives in a patch, and this is the hook it hangs on.
function Script:getAttackIgnored(info, attackBase)
	return 0
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

	return getBonusValueOfSubtype(info.attacker, info.attackerBonuses, "PERCENTAGE_DAMAGE_BOOST", subtype) / 100
end

function Script:getBlessFactor(info)
	return getBonusValueOfType(info.attacker, info.attackerBonuses, "GENERAL_DAMAGE_PREMY") / 100
end

function Script:getLuckFactor(info)
	return info.luckyStrike and 1.0 or 0
end

function Script:getJoustingFactor(info)
	if info.chargeDistance <= 0 then return 0 end
	if not hasBonusOfType(info.attackerBonuses, "JOUSTING") then return 0 end
	if hasBonusOfType(info.defenderBonuses, "CHARGE_IMMUNITY") then return 0 end

	return info.chargeDistance * getBonusValueOfType(info.attacker, info.attackerBonuses, "JOUSTING") / 100
end

function Script:getDeathBlowFactor(info)
	return info.deathBlow and 1.0 or 0
end

function Script:getDoubleDamageFactor(info)
	if not info.doubleDamage then return 0 end

	local ownKey = info.attacker:getCreature():getJsonKey()

	return getBonusValueOfSubtype(info.attacker, info.attackerBonuses, "BONUS_DAMAGE_PERCENTAGE", ownKey) / 100
end

--- Hate of the creature being struck.
function Script:getHateCreatureFactor(info)
	if not hasBonusOfType(info.attackerBonuses, "HATE") then return 0 end

	local hatedKey = info.defender:getCreature():getJsonKey()

	return getBonusValueOfSubtype(info.attacker, info.attackerBonuses, "HATE", hatedKey) / 100
end

--- Armorer and everything else that lessens every kind of blow, other than being petrified.
function Script:getArmorerFactor(info)
	if not hasBonusOfType(info.defenderBonuses, "GENERAL_DAMAGE_REDUCTION") then return 0 end

	return -info.defender:getBonuses({type = "GENERAL_DAMAGE_REDUCTION", subtype = DAMAGE_TYPE_ALL}):filter(function(bonus)
		return bonus:getSource() ~= ENUM.BonusSource.spellEffect
	end):totalValue() / 100
end

--- Shield and air shield: each lessens one kind of blow and ignores the other.
function Script:getMagicShieldFactor(info)
	local subtype = info.shooting and DAMAGE_TYPE_RANGED or DAMAGE_TYPE_MELEE

	return -getBonusValueOfSubtype(info.defender, info.defenderBonuses, "GENERAL_DAMAGE_REDUCTION", subtype) / 100
end

--- Shooting too far, or shooting at all with something meant for melee.
function Script:getRangePenaltyFactor(info)
	if info.shooting then
		if info.battle:hasDistancePenalty(info.attacker, info.defender, info.attackerHex, info.defenderHex) then return -0.5 end

		return 0
	end

	if not hasBonusOfType(info.attackerBonuses, "NO_MELEE_PENALTY") and info.attacker:isShooter() then return -0.5 end

	return 0
end

function Script:getObstacleFactor(info)
	if not info.shooting then return 0 end
	if info.battle:hasWallPenalty(info.attacker, info.defender, info.attackerHex, info.defenderHex) then return -0.5 end

	return 0
end

--- Blindness and paralysis, which leave their bearer striking feebly.
function Script:getBlindParalysisFactor(info)
	return -getBonusValueOfTypeAndRange(info.attacker, info.attackerBonuses, "GENERAL_ATTACK_REDUCTION", info.shooting) / 100
end

function Script:getUnluckyFactor(info)
	return info.unluckyStrike and -0.5 or 0
end

--- Forgetfulness, which only spoils shooting.
function Script:getForgetfulnessFactor(info)
	if not info.shooting then return 0 end

	if not hasBonusOfType(info.attackerBonuses, "FORGETFULL") then return 0 end

	return -math.min(getBonusValueOfType(info.attacker, info.attackerBonuses, "FORGETFULL"), 100) / 100
end

--- A petrified creature takes half of everything, which is not armour and does not count as it.
function Script:getPetrificationFactor(info)
	if not hasBonusOfType(info.defenderBonuses, "GENERAL_DAMAGE_REDUCTION") then return 0 end

	return -info.defender:getBonusesValue({
		type = "GENERAL_DAMAGE_REDUCTION",
		subtype = DAMAGE_TYPE_ALL,
		sourceType = ENUM.BonusSource.spellEffect
	}) / 100
end

--- Every factor of this blow, by name. A patch adds its own with `addDamageFactor`.
function Script:getFactors()
	return self.damageFactors
end

-- ---- what comes out of it all ----------------------------------------------------------------

--- Most damage the target can take from one blow, whatever the blow is worth. Nothing of the game
--- caps it, so the rule lives in a patch and this is the hook it hangs on.
function Script:getDamageCap(info)
	return math.huge
end

--- How many creatures blows of this size kill. Both ends of the range are answered at once, since
--- what decides it - the health and the size of the target - is the same for either.
function Script:getCasualties(info, lowDamage, highDamage)
	local firstHealth = info.defender:getFirstHPleft()
	local creatureHealth = info.defender:getMaxHealth()
	local count = info.defender:getCount()

	local function killedBy(damage)
		if damage < firstHealth then return 0 end

		return math.min(1 + math.floor((damage - firstHealth) / creatureHealth), count)
	end

	return killedBy(lowDamage), killedBy(highDamage)
end

function Script:calculate(battle, info)
	-- the battle answers the queries that depend on where the blow happens; it rides along with the
	-- rest of the attack rather than in a global, which a script shared between threads must not have
	info.battle = battle

	local baseMin, baseMax = self:getBaseDamage(info)

	local raising = 1.0
	local lowering = 1.0

	for _, method in ipairs(self:getFactors()) do
		local factor = self[method](self, info)

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

	local killsMin, killsMax = self:getCasualties(info, damageMin, damageMax)

	return {
		damage = { min = damageMin, max = damageMax },
		kills = { min = killsMin, max = killsMax },
		-- what the blow would have been worth had the target no defences at all, which is what an
		-- ability reflecting a strike works from
		damageBeforeDefense = { min = apply(baseMin, raising), max = apply(baseMax, raising) }
	}
end

-- ---- what this calculator is made of ---------------------------------------------------------

for _, type in ipairs({
	"ALWAYS_MINIMUM_DAMAGE", "ALWAYS_MAXIMUM_DAMAGE", "IN_FRENZY", "KING", "SLAYER",
	"ENEMY_DEFENCE_REDUCTION", "PERCENTAGE_DAMAGE_BOOST", "GENERAL_DAMAGE_PREMY", "JOUSTING",
	"CHARGE_IMMUNITY", "BONUS_DAMAGE_PERCENTAGE", "HATE", "GENERAL_DAMAGE_REDUCTION",
	"NO_MELEE_PENALTY", "GENERAL_ATTACK_REDUCTION", "FORGETFULL"
}) do
	Script:declareBonus(type)
end

for _, factor in ipairs({
	"getAttackDefenseFactor", "getOffenseArcheryFactor", "getBlessFactor", "getLuckFactor",
	"getJoustingFactor", "getDeathBlowFactor", "getDoubleDamageFactor", "getHateCreatureFactor",
	"getArmorerFactor", "getMagicShieldFactor", "getRangePenaltyFactor", "getObstacleFactor",
	"getBlindParalysisFactor", "getUnluckyFactor", "getForgetfulnessFactor", "getPetrificationFactor"
}) do
	Script:addDamageFactor(factor)
end

return Script
