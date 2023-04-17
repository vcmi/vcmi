/*
 * HeroBonus.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "GameConstants.h"
#include "JsonNode.h"
#include "battle/BattleHex.h"
#include <limits>

VCMI_LIB_NAMESPACE_BEGIN

class CCreature;
struct Bonus;
class IBonusBearer;
class CBonusSystemNode;
class ILimiter;
class IPropagator;
class IUpdater;
class BonusList;

using TBonusListPtr = std::shared_ptr<BonusList>;
using TConstBonusListPtr = std::shared_ptr<const BonusList>;
using TLimiterPtr = std::shared_ptr<ILimiter>;
using TPropagatorPtr = std::shared_ptr<IPropagator>;
using TUpdaterPtr = std::shared_ptr<IUpdater>;
using TNodes = std::set<CBonusSystemNode *>;
using TCNodes = std::set<const CBonusSystemNode *>;
using TNodesVector = std::vector<CBonusSystemNode *>;

class CSelector : std::function<bool(const Bonus*)>
{
	using TBase = std::function<bool(const Bonus*)>;
public:
	CSelector() = default;
	template<typename T>
	CSelector(const T &t,	//SFINAE trick -> include this c-tor in overload resolution only if parameter is class
							//(includes functors, lambdas) or function. Without that VC is going mad about ambiguities.
		typename std::enable_if < boost::mpl::or_ < std::is_class<T>, std::is_function<T >> ::value>::type *dummy = nullptr)
		: TBase(t)
	{}

	CSelector(std::nullptr_t)
	{}

	CSelector And(CSelector rhs) const
	{
		//lambda may likely outlive "this" (it can be even a temporary) => we copy the OBJECT (not pointer)
		auto thisCopy = *this;
		return [thisCopy, rhs](const Bonus *b) mutable { return thisCopy(b) && rhs(b); };
	}
	CSelector Or(CSelector rhs) const
	{
		auto thisCopy = *this;
		return [thisCopy, rhs](const Bonus *b) mutable { return thisCopy(b) || rhs(b); };
	}

	CSelector Not() const
	{
		auto thisCopy = *this;
		return [thisCopy](const Bonus *b) mutable { return !thisCopy(b); };
	}

	bool operator()(const Bonus *b) const
	{
		return TBase::operator()(b);
	}

	operator bool() const
	{
		return !!static_cast<const TBase&>(*this);
	}
};

class DLL_LINKAGE CBonusProxy
{
public:
	CBonusProxy(const IBonusBearer * Target, CSelector Selector);
	CBonusProxy(const CBonusProxy & other);
	CBonusProxy(CBonusProxy && other) noexcept;

	CBonusProxy & operator=(CBonusProxy && other) noexcept;
	CBonusProxy & operator=(const CBonusProxy & other);
	const BonusList * operator->() const;
	TConstBonusListPtr getBonusList() const;

protected:
	CSelector selector;
	const IBonusBearer * target;
	mutable int64_t bonusListCachedLast;
	mutable TConstBonusListPtr bonusList[2];
	mutable int currentBonusListIndex;
	mutable boost::mutex swapGuard;
	void swapBonusList(TConstBonusListPtr other) const;
};

class DLL_LINKAGE CTotalsProxy : public CBonusProxy
{
public:
	CTotalsProxy(const IBonusBearer * Target, CSelector Selector, int InitialValue);
	CTotalsProxy(const CTotalsProxy & other);
	CTotalsProxy(CTotalsProxy && other) = delete;

	CTotalsProxy & operator=(const CTotalsProxy & other) = default;
	CTotalsProxy & operator=(CTotalsProxy && other) = delete;

	int getMeleeValue() const;
	int getRangedValue() const;
	int getValue() const;
	/**
	Returns total value of all selected bonuses and sets bonusList as a pointer to the list of selected bonuses
	@param bonusList is the out list of all selected bonuses
	@return total value of all selected bonuses and 0 otherwise
	*/
	int getValueAndList(TConstBonusListPtr & bonusList) const;

private:
	int initialValue;

	mutable int64_t valueCachedLast = 0;
	mutable int value = 0;

	mutable int64_t meleeCachedLast;
	mutable int meleeValue;

	mutable int64_t rangedCachedLast;
	mutable int rangedValue;
};

class DLL_LINKAGE CCheckProxy
{
public:
	CCheckProxy(const IBonusBearer * Target, CSelector Selector);
	CCheckProxy(const CCheckProxy & other);
	CCheckProxy& operator= (const CCheckProxy & other) = default;

	bool getHasBonus() const;

private:
	const IBonusBearer * target;
	CSelector selector;

	mutable int64_t cachedLast;
	mutable bool hasBonus;
};

class DLL_LINKAGE CAddInfo : public std::vector<si32>
{
public:
	enum { NONE = -1 };

	CAddInfo();
	CAddInfo(si32 value);

	bool operator==(si32 value) const;
	bool operator!=(si32 value) const;

	si32 & operator[](size_type pos);
	si32 operator[](size_type pos) const;

	std::string toString() const;
	JsonNode toJsonNode() const;
};

#define BONUS_TREE_DESERIALIZATION_FIX if(!h.saving && h.smartPointerSerialization) deserializationFix();

#define BONUS_LIST										\
	BONUS_NAME(NONE) 									\
	BONUS_NAME(LEVEL_COUNTER) /* for commander artifacts*/ \
	BONUS_NAME(MOVEMENT) /*Subtype is 1 - land, 0 - sea*/ \
	BONUS_NAME(MORALE) \
	BONUS_NAME(LUCK) \
	BONUS_NAME(PRIMARY_SKILL) /*uses subtype to pick skill; additional info if set: 1 - only melee, 2 - only distance*/  \
	BONUS_NAME(SIGHT_RADIUS) \
	BONUS_NAME(MANA_REGENERATION) /*points per turn apart from normal (1 + mysticism)*/  \
	BONUS_NAME(FULL_MANA_REGENERATION) /*all mana points are replenished every day*/  \
	BONUS_NAME(NONEVIL_ALIGNMENT_MIX) /*good and neutral creatures can be mixed without morale penalty*/  \
	BONUS_NAME(SURRENDER_DISCOUNT) /*%*/  \
	BONUS_NAME(STACKS_SPEED)  /*additional info - percent of speed bonus applied after direct bonuses; >0 - added, <0 - subtracted to this part*/ \
	BONUS_NAME(FLYING_MOVEMENT) /*value - penalty percentage*/ \
	BONUS_NAME(SPELL_DURATION) \
	BONUS_NAME(AIR_SPELL_DMG_PREMY) \
	BONUS_NAME(EARTH_SPELL_DMG_PREMY) \
	BONUS_NAME(FIRE_SPELL_DMG_PREMY) \
	BONUS_NAME(WATER_SPELL_DMG_PREMY) \
	BONUS_NAME(WATER_WALKING) /*value - penalty percentage*/ \
	BONUS_NAME(NEGATE_ALL_NATURAL_IMMUNITIES) \
	BONUS_NAME(STACK_HEALTH) \
	BONUS_NAME(FIRE_SPELLS) \
	BONUS_NAME(AIR_SPELLS) \
	BONUS_NAME(WATER_SPELLS) \
	BONUS_NAME(EARTH_SPELLS) \
	BONUS_NAME(GENERATE_RESOURCE) /*daily value, uses subtype (resource type)*/  \
	BONUS_NAME(CREATURE_GROWTH) /*for legion artifacts: value - week growth bonus, subtype - monster level if aplicable*/  \
	BONUS_NAME(WHIRLPOOL_PROTECTION) /*hero won't lose army when teleporting through whirlpool*/  \
	BONUS_NAME(SPELL) /*hero knows spell, val - skill level (0 - 3), subtype - spell id*/  \
	BONUS_NAME(SPELLS_OF_LEVEL) /*hero knows all spells of given level, val - skill level; subtype - level*/  \
	BONUS_NAME(BATTLE_NO_FLEEING) /*for shackles of war*/ \
	BONUS_NAME(MAGIC_SCHOOL_SKILL) /* //eg. for magic plains terrain, subtype: school of magic (0 - all, 1 - fire, 2 - air, 4 - water, 8 - earth), value - level*/ \
	BONUS_NAME(FREE_SHOOTING) /*stacks can shoot even if otherwise blocked (sharpshooter's bow effect)*/ \
	BONUS_NAME(OPENING_BATTLE_SPELL) /*casts a spell at expert level at beginning of battle, val - spell power, subtype - spell id*/ \
	BONUS_NAME(IMPROVED_NECROMANCY) /* raise more powerful creatures: subtype - creature type raised, addInfo - [required necromancy level, required stack level], val - necromancy level for this purpose */ \
	BONUS_NAME(CREATURE_GROWTH_PERCENT) /*increases growth of all units in all towns, val - percentage*/ \
	BONUS_NAME(FREE_SHIP_BOARDING) /*movement points preserved with ship boarding and landing*/  \
	BONUS_NAME(FLYING)									\
	BONUS_NAME(SHOOTER)									\
	BONUS_NAME(CHARGE_IMMUNITY)							\
	BONUS_NAME(ADDITIONAL_ATTACK)						\
	BONUS_NAME(UNLIMITED_RETALIATIONS)					\
	BONUS_NAME(NO_MELEE_PENALTY)						\
	BONUS_NAME(JOUSTING) /*for champions*/				\
	BONUS_NAME(HATE) /*eg. angels hate devils, subtype - ID of hated creature, val - damage bonus percent */ \
	BONUS_NAME(KING) /* val - required slayer bonus val to affect */\
	BONUS_NAME(MAGIC_RESISTANCE) /*in % (value)*/		\
	BONUS_NAME(CHANGES_SPELL_COST_FOR_ALLY) /*in mana points (value) , eg. mage*/ \
	BONUS_NAME(CHANGES_SPELL_COST_FOR_ENEMY) /*in mana points (value) , eg. pegasus */ \
	BONUS_NAME(SPELL_AFTER_ATTACK) /* subtype - spell id, value - chance %, addInfo[0] - level, addInfo[1] -> [0 - all attacks, 1 - shot only, 2 - melee only] */ \
	BONUS_NAME(SPELL_BEFORE_ATTACK) /* subtype - spell id, value - chance %, addInfo[0] - level, addInfo[1] -> [0 - all attacks, 1 - shot only, 2 - melee only] */ \
	BONUS_NAME(SPELL_RESISTANCE_AURA) /*eg. unicorns, value - resistance bonus in % for adjacent creatures*/ \
	BONUS_NAME(LEVEL_SPELL_IMMUNITY) /*creature is immune to all spell with level below or equal to value of this bonus */ \
	BONUS_NAME(BLOCK_MAGIC_ABOVE) /*blocks casting spells of the level > value */ \
	BONUS_NAME(BLOCK_ALL_MAGIC) /*blocks casting spells*/ \
	BONUS_NAME(TWO_HEX_ATTACK_BREATH) /*eg. dragons*/	\
	BONUS_NAME(SPELL_DAMAGE_REDUCTION) /*eg. golems; value - reduction in %, subtype - spell school; -1 - all, 0 - air, 1 - fire, 2 - water, 3 - earth*/ \
	BONUS_NAME(NO_WALL_PENALTY)							\
	BONUS_NAME(NON_LIVING) /*eg. golems, cannot be rised or healed, only neutral morale */				\
	BONUS_NAME(RANDOM_SPELLCASTER) /*eg. master genie, val - level*/ \
	BONUS_NAME(BLOCKS_RETALIATION) /*eg. naga*/			\
	BONUS_NAME(SPELL_IMMUNITY) /*subid - spell id*/		\
	BONUS_NAME(MANA_CHANNELING) /*value in %, eg. familiar*/ \
	BONUS_NAME(SPELL_LIKE_ATTACK) /*subtype - spell, value - spell level; range is taken from spell, but damage from creature; eg. magog*/ \
	BONUS_NAME(THREE_HEADED_ATTACK) /*eg. cerberus*/	\
	BONUS_NAME(GENERAL_DAMAGE_PREMY)						\
	BONUS_NAME(FIRE_IMMUNITY)	/*subtype 0 - all, 1 - all except positive, 2 - only damage spells*/						\
	BONUS_NAME(WATER_IMMUNITY)							\
	BONUS_NAME(EARTH_IMMUNITY)							\
	BONUS_NAME(AIR_IMMUNITY)							\
	BONUS_NAME(MIND_IMMUNITY)							\
	BONUS_NAME(FIRE_SHIELD)								\
	BONUS_NAME(UNDEAD)									\
	BONUS_NAME(HP_REGENERATION) /*creature regenerates val HP every new round*/					\
	BONUS_NAME(MANA_DRAIN) /*value - spell points per turn*/ \
	BONUS_NAME(LIFE_DRAIN)								\
	BONUS_NAME(DOUBLE_DAMAGE_CHANCE) /*value in %, eg. dread knight*/ \
	BONUS_NAME(RETURN_AFTER_STRIKE)						\
	BONUS_NAME(SPELLCASTER) /*subtype - spell id, value - level of school, additional info - weighted chance. use SPECIFIC_SPELL_POWER, CREATURE_SPELL_POWER or CREATURE_ENCHANT_POWER for calculating the power*/ \
	BONUS_NAME(CATAPULT)								\
	BONUS_NAME(ENEMY_DEFENCE_REDUCTION) /*in % (value) eg. behemots*/ \
	BONUS_NAME(GENERAL_DAMAGE_REDUCTION) /* shield / air shield effect, also armorer skill/petrify effect for subtype -1*/ \
	BONUS_NAME(GENERAL_ATTACK_REDUCTION) /*eg. while stoned or blinded - in %,// subtype not used, use ONLY_MELEE_FIGHT / DISTANCE_FIGHT*/ \
	BONUS_NAME(DEFENSIVE_STANCE) /* val - bonus to defense while defending */ \
	BONUS_NAME(ATTACKS_ALL_ADJACENT) /*eg. hydra*/		\
	BONUS_NAME(MORE_DAMAGE_FROM_SPELL) /*value - damage increase in %, subtype - spell id*/ \
	BONUS_NAME(FEAR)									\
	BONUS_NAME(FEARLESS)								\
	BONUS_NAME(NO_DISTANCE_PENALTY)						\
	BONUS_NAME(ENCHANTER)/* for Enchanter spells, val - skill level, subtype - spell id, additionalInfo - cooldown */ \
	BONUS_NAME(HEALER)									\
	BONUS_NAME(SIEGE_WEAPON)							\
	BONUS_NAME(HYPNOTIZED)								\
	BONUS_NAME(NO_RETALIATION) /*temporary bonus for basilisk, unicorn and scorpicore paralyze*/\
	BONUS_NAME(ADDITIONAL_RETALIATION) /*value - number of additional retaliations*/ \
	BONUS_NAME(MAGIC_MIRROR) /* value - chance of redirecting in %*/ \
	BONUS_NAME(ALWAYS_MINIMUM_DAMAGE) /*unit does its minimum damage from range; subtype: -1 - any attack, 0 - melee, 1 - ranged, value: additional damage penalty (it'll subtracted from dmg), additional info - multiplicative anti-bonus for dmg in % [eg 20 means that creature will inflict 80% of normal minimal dmg]*/ \
	BONUS_NAME(ALWAYS_MAXIMUM_DAMAGE) /*eg. bless effect, subtype: -1 - any attack, 0 - melee, 1 - ranged, value: additional damage, additional info - multiplicative bonus for dmg in %*/ \
	BONUS_NAME(ATTACKS_NEAREST_CREATURE) /*while in berserk*/ \
	BONUS_NAME(IN_FRENZY) /*value - level*/				\
	BONUS_NAME(SLAYER) /*value - level*/				\
	BONUS_NAME(FORGETFULL) /*forgetfulness spell effect, value - level*/ \
	BONUS_NAME(NOT_ACTIVE) /* subtype - spell ID (paralyze, blind, stone gaze) for graphical effect*/ 								\
	BONUS_NAME(NO_LUCK) /*eg. when fighting on cursed ground*/	\
	BONUS_NAME(NO_MORALE) /*eg. when fighting on cursed ground*/ \
	BONUS_NAME(DARKNESS) /*val = radius */ \
	BONUS_NAME(SPECIAL_SPELL_LEV) /*subtype = id, val = value per level in percent*/\
	BONUS_NAME(SPELL_DAMAGE) /*val = value, now works for sorcery*/\
	BONUS_NAME(SPECIFIC_SPELL_DAMAGE) /*subtype = id of spell, val = value*/\
	BONUS_NAME(SPECIAL_PECULIAR_ENCHANT) /*blesses and curses with id = val dependent on unit's level, subtype = 0 or 1 for Coronius*/\
	BONUS_NAME(SPECIAL_UPGRADE) /*subtype = base, additionalInfo = target */\
	BONUS_NAME(DRAGON_NATURE) \
	BONUS_NAME(CREATURE_DAMAGE)/*subtype 0 = both, 1 = min, 2 = max*/\
	BONUS_NAME(EXP_MULTIPLIER)/* val - percent of additional exp gained by stack/commander (base value 100)*/\
	BONUS_NAME(SHOTS)\
	BONUS_NAME(DEATH_STARE) /*subtype 0 - gorgon, 1 - commander*/\
	BONUS_NAME(POISON) /*val - max health penalty from poison possible*/\
	BONUS_NAME(BIND_EFFECT) /*doesn't do anything particular, works as a marker)*/\
	BONUS_NAME(ACID_BREATH) /*additional val damage per creature after attack, additional info - chance in percent*/\
	BONUS_NAME(RECEPTIVE) /*accepts friendly spells even with immunity*/\
	BONUS_NAME(DIRECT_DAMAGE_IMMUNITY) /*direct damage spells, that is*/\
	BONUS_NAME(CASTS) /*how many times creature can cast activated spell*/ \
	BONUS_NAME(SPECIFIC_SPELL_POWER) /* value used for Thunderbolt and Resurrection cast by units, subtype - spell id */\
	BONUS_NAME(CREATURE_SPELL_POWER) /* value per unit, divided by 100 (so faerie Dragons have 800)*/ \
	BONUS_NAME(CREATURE_ENCHANT_POWER) /* total duration of spells cast by creature */ \
	BONUS_NAME(ENCHANTED) /* permanently enchanted with spell subID of level = val, if val > 3 then spell is mass and has level of val-3*/ \
	BONUS_NAME(REBIRTH) /* val - percent of life restored, subtype = 0 - regular, 1 - at least one unit (sacred Phoenix) */\
	BONUS_NAME(ADDITIONAL_UNITS) /*val of units with id = subtype will be added to hero's army at the beginning of battle */\
	BONUS_NAME(SPOILS_OF_WAR) /*val * 10^-6 * gained exp resources of subtype will be given to hero after battle*/\
	BONUS_NAME(BLOCK)\
	BONUS_NAME(DISGUISED) /* subtype - spell level */\
	BONUS_NAME(VISIONS) /* subtype - spell level */\
	BONUS_NAME(NO_TERRAIN_PENALTY) /* subtype - terrain type */\
	BONUS_NAME(SOUL_STEAL) /*val - number of units gained per enemy killed, subtype = 0 - gained units survive after battle, 1 - they do not*/ \
	BONUS_NAME(TRANSMUTATION) /*val - chance to trigger in %, subtype = 0 - resurrection based on HP, 1 - based on unit count, additional info - target creature ID (attacker default)*/\
	BONUS_NAME(SUMMON_GUARDIANS) /*val - amount in % of stack count, subtype = creature ID*/\
	BONUS_NAME(CATAPULT_EXTRA_SHOTS) /*val - power of catapult effect, requires CATAPULT bonus to work*/\
	BONUS_NAME(RANGED_RETALIATION) /*allows shooters to perform ranged retaliation*/\
	BONUS_NAME(BLOCKS_RANGED_RETALIATION) /*disallows ranged retaliation for shooter unit, BLOCKS_RETALIATION bonus is for melee retaliation only*/\
	BONUS_NAME(MANUAL_CONTROL) /* manually control warmachine with id = subtype, chance = val */  \
	BONUS_NAME(WIDE_BREATH) /* initial desigh: dragon breath affecting multiple nearby hexes */\
	BONUS_NAME(FIRST_STRIKE) /* first counterattack, then attack if possible */\
	BONUS_NAME(SYNERGY_TARGET) /* dummy skill for alternative upgrades mod */\
	BONUS_NAME(SHOOTS_ALL_ADJACENT) /* H4 Cyclops-like shoot (attacks all hexes neighboring with target) without spell-like mechanics */\
	BONUS_NAME(BLOCK_MAGIC_BELOW) /*blocks casting spells of the level < value */ \
	BONUS_NAME(DESTRUCTION) /*kills extra units after hit, subtype = 0 - kill percentage of units, 1 - kill amount, val = chance in percent to trigger, additional info - amount/percentage to kill*/ \
	BONUS_NAME(SPECIAL_CRYSTAL_GENERATION) /*crystal dragon crystal generation*/ \
	BONUS_NAME(NO_SPELLCAST_BY_DEFAULT) /*spellcast will not be default attack option for this creature*/ \
	BONUS_NAME(GARGOYLE) /* gargoyle is special than NON_LIVING, cannot be rised or healed */ \
	BONUS_NAME(SPECIAL_ADD_VALUE_ENCHANT) /*specialty spell like Aenin has, increased effect of spell, additionalInfo = value to add*/\
	BONUS_NAME(SPECIAL_FIXED_VALUE_ENCHANT) /*specialty spell like Melody has, constant spell effect (i.e. 3 luck), additionalInfo = value to fix.*/\
	BONUS_NAME(TOWN_MAGIC_WELL) /*one-time pseudo-bonus to implement Magic Well in the town*/\
	BONUS_NAME(LIMITED_SHOOTING_RANGE) /*limits range of shooting creatures, doesn't adjust any other mechanics (half vs full damage etc). val - range in hexes, additional info - optional new range for broken arrow mechanic */\
	BONUS_NAME(LEARN_BATTLE_SPELL_CHANCE) /*skill-agnostic eagle eye chance. subtype = 0 - from enemy, 1 - TODO: from entire battlefield*/\
	BONUS_NAME(LEARN_BATTLE_SPELL_LEVEL_LIMIT) /*skill-agnostic eagle eye limit, subtype - school (-1 for all), others TODO*/\
	BONUS_NAME(PERCENTAGE_DAMAGE_BOOST) /*skill-agnostic archery and offence, subtype is 0 for offence and 1 for archery*/\
	BONUS_NAME(LEARN_MEETING_SPELL_LIMIT) /*skill-agnostic scholar, subtype is -1 for all, TODO for others (> 0)*/\
	BONUS_NAME(ROUGH_TERRAIN_DISCOUNT) /*skill-agnostic pathfinding*/\
	BONUS_NAME(WANDERING_CREATURES_JOIN_BONUS) /*skill-agnostic diplomacy*/\
	BONUS_NAME(BEFORE_BATTLE_REPOSITION) /*skill-agnostic tactics, bonus for allowing tactics*/\
	BONUS_NAME(BEFORE_BATTLE_REPOSITION_BLOCK) /*skill-agnostic tactics, bonus for blocking opposite tactics. For now donble side tactics is TODO.*/\
	BONUS_NAME(HERO_EXPERIENCE_GAIN_PERCENT) /*skill-agnostic learning, and we can use it as a global effect also*/\
	BONUS_NAME(UNDEAD_RAISE_PERCENTAGE) /*Percentage of killed enemy creatures to be raised after battle as undead*/\
	BONUS_NAME(MANA_PER_KNOWLEDGE) /*Percentage rate of translating 10 hero knowledge to mana, used to intelligence and global bonus*/\
	BONUS_NAME(HERO_GRANTS_ATTACKS) /*If hero can grant additional attacks to creature, value is number of attacks, subtype is creatureID*/\
	BONUS_NAME(BONUS_DAMAGE_PERCENTAGE) /*If hero can grant conditional damage to creature, value is percentage, subtype is creatureID*/\
	BONUS_NAME(BONUS_DAMAGE_CHANCE) /*If hero can grant additional damage to creature, value is chance, subtype is creatureID*/\
	BONUS_NAME(MAX_LEARNABLE_SPELL_LEVEL) /*This can work as wisdom before. val = max learnable spell level*/\
	/* end of list */


#define BONUS_SOURCE_LIST \
	BONUS_SOURCE(ARTIFACT)\
	BONUS_SOURCE(ARTIFACT_INSTANCE)\
	BONUS_SOURCE(OBJECT)\
	BONUS_SOURCE(CREATURE_ABILITY)\
	BONUS_SOURCE(TERRAIN_NATIVE)\
	BONUS_SOURCE(TERRAIN_OVERLAY)\
	BONUS_SOURCE(SPELL_EFFECT)\
	BONUS_SOURCE(TOWN_STRUCTURE)\
	BONUS_SOURCE(HERO_BASE_SKILL)\
	BONUS_SOURCE(SECONDARY_SKILL)\
	BONUS_SOURCE(HERO_SPECIAL)\
	BONUS_SOURCE(ARMY)\
	BONUS_SOURCE(CAMPAIGN_BONUS)\
	BONUS_SOURCE(SPECIAL_WEEK)\
	BONUS_SOURCE(STACK_EXPERIENCE)\
	BONUS_SOURCE(COMMANDER) /*TODO: consider using simply STACK_INSTANCE */\
	BONUS_SOURCE(GLOBAL) /*used for base bonuses which all heroes or all stacks should have*/\
	BONUS_SOURCE(OTHER) /*used for defensive stance and default value of spell level limit*/

#define BONUS_VALUE_LIST \
	BONUS_VALUE(ADDITIVE_VALUE)\
	BONUS_VALUE(BASE_NUMBER)\
	BONUS_VALUE(PERCENT_TO_ALL)\
	BONUS_VALUE(PERCENT_TO_BASE)\
	BONUS_VALUE(PERCENT_TO_SOURCE) /*Adds value only to bonuses with same source*/\
	BONUS_VALUE(PERCENT_TO_TARGET_TYPE) /*Adds value only to bonuses with SourceType target*/\
	BONUS_VALUE(INDEPENDENT_MAX) /*used for SPELL bonus */\
	BONUS_VALUE(INDEPENDENT_MIN) //used for SECONDARY_SKILL_PREMY bonus

/// Struct for handling bonuses of several types. Can be transferred to any hero
struct DLL_LINKAGE Bonus : public std::enable_shared_from_this<Bonus>
{
	enum { EVERY_TYPE = -1 };

	enum BonusType
	{
#define BONUS_NAME(x) x,
		BONUS_LIST
#undef BONUS_NAME
	};
	enum BonusDuration //when bonus is automatically removed
	{
		PERMANENT = 1,
		ONE_BATTLE = 2, //at the end of battle
		ONE_DAY = 4,   //at the end of day
		ONE_WEEK = 8, //at the end of week (bonus lasts till the end of week, thats NOT 7 days
		N_TURNS = 16, //used during battles, after battle bonus is always removed
		N_DAYS = 32,
		UNTIL_BEING_ATTACKED = 64, /*removed after attack and counterattacks are performed*/
		UNTIL_ATTACK = 128, /*removed after attack and counterattacks are performed*/
		STACK_GETS_TURN = 256, /*removed when stack gets its turn - used for defensive stance*/
		COMMANDER_KILLED = 512
	};
	enum BonusSource
	{
#define BONUS_SOURCE(x) x,
		BONUS_SOURCE_LIST
#undef BONUS_SOURCE
		NUM_BONUS_SOURCE /*This is a dummy value, which will be always last*/
	};

	enum LimitEffect
	{
		NO_LIMIT = 0,
		ONLY_DISTANCE_FIGHT=1, ONLY_MELEE_FIGHT, //used to mark bonuses for attack/defense primary skills from spells like Precision (distance only)
	};

	enum ValueType
	{
#define BONUS_VALUE(x) x,
		BONUS_VALUE_LIST
#undef BONUS_VALUE
	};

	ui16 duration = PERMANENT; //uses BonusDuration values
	si16 turnsRemain = 0; //used if duration is N_TURNS, N_DAYS or ONE_WEEK

	BonusType type = NONE; //uses BonusType values - says to what is this bonus - 1 byte
	TBonusSubtype subtype = -1; //-1 if not applicable - 4 bytes

	BonusSource source = OTHER; //source type" uses BonusSource values - what gave that bonus
	BonusSource targetSourceType;//Bonuses of what origin this amplifies, uses BonusSource values. Needed for PERCENT_TO_TARGET_TYPE.
	si32 val = 0;
	ui32 sid = 0; //source id: id of object/artifact/spell
	ValueType valType = ADDITIVE_VALUE;
	std::string stacking; // bonuses with the same stacking value don't stack (e.g. Angel/Archangel morale bonus)

	CAddInfo additionalInfo;
	LimitEffect effectRange = NO_LIMIT; //if not NO_LIMIT, bonus will be omitted by default

	TLimiterPtr limiter;
	TPropagatorPtr propagator;
	TUpdaterPtr updater;
	TUpdaterPtr propagationUpdater;

	std::string description;

	Bonus(BonusDuration Duration, BonusType Type, BonusSource Src, si32 Val, ui32 ID, std::string Desc, si32 Subtype=-1);
	Bonus(BonusDuration Duration, BonusType Type, BonusSource Src, si32 Val, ui32 ID, si32 Subtype=-1, ValueType ValType = ADDITIVE_VALUE);
	Bonus() = default;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & duration;
		h & type;
		h & subtype;
		h & source;
		h & val;
		h & sid;
		h & description;
		h & additionalInfo;
		h & turnsRemain;
		h & valType;
		h & stacking;
		h & effectRange;
		h & limiter;
		h & propagator;
		h & updater;
		h & propagationUpdater;
		h & targetSourceType;
	}

	template <typename Ptr>
	static bool compareByAdditionalInfo(const Ptr& a, const Ptr& b)
	{
		return a->additionalInfo < b->additionalInfo;
	}
	static bool NDays(const Bonus *hb)
	{
		return hb->duration & Bonus::N_DAYS;
	}
	static bool NTurns(const Bonus *hb)
	{
		return hb->duration & Bonus::N_TURNS;
	}
	static bool OneDay(const Bonus *hb)
	{
		return hb->duration & Bonus::ONE_DAY;
	}
	static bool OneWeek(const Bonus *hb)
	{
		return hb->duration & Bonus::ONE_WEEK;
	}
	static bool OneBattle(const Bonus *hb)
	{
		return hb->duration & Bonus::ONE_BATTLE;
	}
	static bool Permanent(const Bonus *hb)
	{
		return hb->duration & Bonus::PERMANENT;
	}
	static bool UntilGetsTurn(const Bonus *hb)
	{
		return hb->duration & Bonus::STACK_GETS_TURN;
	}
	static bool UntilAttack(const Bonus *hb)
	{
		return hb->duration & Bonus::UNTIL_ATTACK;
	}
	static bool UntilBeingAttacked(const Bonus *hb)
	{
		return hb->duration & Bonus::UNTIL_BEING_ATTACKED;
	}
	static bool UntilCommanderKilled(const Bonus *hb)
	{
		return hb->duration & Bonus::COMMANDER_KILLED;
	}
	inline bool operator == (const BonusType & cf) const
	{
		return type == cf;
	}
	inline void operator += (const ui32 Val) //no return
	{
		val += Val;
	}
	STRONG_INLINE static ui32 getSid32(ui32 high, ui32 low)
	{
		return (high << 16) + low;
	}

	STRONG_INLINE static ui32 getHighFromSid32(ui32 sid)
	{
		return sid >> 16;
	}

	STRONG_INLINE static ui32 getLowFromSid32(ui32 sid)
	{
		return sid & 0x0000FFFF;
	}

	std::string Description(std::optional<si32> customValue = {}) const;
	JsonNode toJsonNode() const;
	std::string nameForBonus() const; // generate suitable name for bonus - e.g. for storing in json struct

	std::shared_ptr<Bonus> addLimiter(const TLimiterPtr & Limiter); //returns this for convenient chain-calls
	std::shared_ptr<Bonus> addPropagator(const TPropagatorPtr & Propagator); //returns this for convenient chain-calls
	std::shared_ptr<Bonus> addUpdater(const TUpdaterPtr & Updater); //returns this for convenient chain-calls
};

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const Bonus &bonus);

struct DLL_LINKAGE BonusParams {
	bool isConverted;
	Bonus::BonusType type = Bonus::NONE;
	TBonusSubtype subtype = -1;
	std::string subtypeStr;
	bool subtypeRelevant = false;
	Bonus::ValueType valueType = Bonus::BASE_NUMBER;
	bool valueTypeRelevant = false;
	si32 val = 0;
	bool valRelevant = false;
	Bonus::BonusSource targetType = Bonus::SECONDARY_SKILL;
	bool targetTypeRelevant = false;

	BonusParams(bool isConverted = true) : isConverted(isConverted) {};
	BonusParams(std::string deprecatedTypeStr, std::string deprecatedSubtypeStr = "", int deprecatedSubtype = 0);
	const JsonNode & toJson();
	CSelector toSelector();
private:
	JsonNode ret;
	bool jsonCreated = false;
};

class DLL_LINKAGE BonusList
{
public:
	using TInternalContainer = std::vector<std::shared_ptr<Bonus>>;

private:
	TInternalContainer bonuses;
	bool belongsToTree;
	void changed() const;

public:
	using const_reference = TInternalContainer::const_reference;
	using value_type = TInternalContainer::value_type;

	using const_iterator = TInternalContainer::const_iterator;
	using iterator = TInternalContainer::iterator;

	BonusList(bool BelongsToTree = false);
	BonusList(const BonusList &bonusList);
	BonusList(BonusList && other) noexcept;
	BonusList& operator=(const BonusList &bonusList);

	// wrapper functions of the STL vector container
	TInternalContainer::size_type size() const { return bonuses.size(); }
	void push_back(const std::shared_ptr<Bonus> & x);
	TInternalContainer::iterator erase (const int position);
	void clear();
	bool empty() const { return bonuses.empty(); }
	void resize(TInternalContainer::size_type sz, const std::shared_ptr<Bonus> & c = nullptr);
	void reserve(TInternalContainer::size_type sz);
	TInternalContainer::size_type capacity() const { return bonuses.capacity(); }
	STRONG_INLINE std::shared_ptr<Bonus> &operator[] (TInternalContainer::size_type n) { return bonuses[n]; }
	STRONG_INLINE const std::shared_ptr<Bonus> &operator[] (TInternalContainer::size_type n) const { return bonuses[n]; }
	std::shared_ptr<Bonus> &back() { return bonuses.back(); }
	std::shared_ptr<Bonus> &front() { return bonuses.front(); }
	const std::shared_ptr<Bonus> &back() const { return bonuses.back(); }
	const std::shared_ptr<Bonus> &front() const { return bonuses.front(); }

	// There should be no non-const access to provide solid,robust bonus caching
	TInternalContainer::const_iterator begin() const { return bonuses.begin(); }
	TInternalContainer::const_iterator end() const { return bonuses.end(); }
	TInternalContainer::size_type operator-=(const std::shared_ptr<Bonus> & i);

	// BonusList functions
	void stackBonuses();
	int totalValue() const;
	void getBonuses(BonusList &out, const CSelector &selector, const CSelector &limit = nullptr) const;
	void getAllBonuses(BonusList &out) const;

	//special find functions
	std::shared_ptr<Bonus> getFirst(const CSelector &select);
	std::shared_ptr<const Bonus> getFirst(const CSelector &select) const;
	int valOfBonuses(const CSelector &select) const;

	// conversion / output
	JsonNode toJsonNode() const;

	// remove_if implementation for STL vector types
	template <class Predicate>
	void remove_if(Predicate pred)
	{
		BonusList newList;
		for(const auto & b : bonuses)
		{
			if (!pred(b.get()))
				newList.push_back(b);
		}
		bonuses.clear();
		bonuses.resize(newList.size());
		std::copy(newList.begin(), newList.end(), bonuses.begin());
	}

	template <class InputIterator>
	void insert(const int position, InputIterator first, InputIterator last);
	void insert(TInternalContainer::iterator position, TInternalContainer::size_type n, const std::shared_ptr<Bonus> & x);

	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & static_cast<TInternalContainer&>(bonuses);
	}

	// C++ for range support
	auto begin () -> decltype (bonuses.begin())
	{
		return bonuses.begin();
	}

	auto end () -> decltype (bonuses.end())
	{
		return bonuses.end();
	}
};

// Extensions for BOOST_FOREACH to enable iterating of BonusList objects
// Don't touch/call this functions
inline BonusList::iterator range_begin(BonusList & x)
{
	return x.begin();
}

inline BonusList::iterator range_end(BonusList & x)
{
	return x.end();
}

inline BonusList::const_iterator range_begin(const BonusList & x)
{
	return x.begin();
}

inline BonusList::const_iterator range_end(const BonusList & x)
{
	return x.end();
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const BonusList &bonusList);

struct BonusLimitationContext
{
	std::shared_ptr<const Bonus> b;
	const CBonusSystemNode & node;
	const BonusList & alreadyAccepted;
	const BonusList & stillUndecided;
};

class DLL_LINKAGE ILimiter
{
public:
	enum class EDecision : uint8_t {ACCEPT, DISCARD, NOT_SURE};

	virtual ~ILimiter() = default;

	virtual EDecision limit(const BonusLimitationContext &context) const; //0 - accept bonus; 1 - drop bonus; 2 - delay (drops eventually)
	virtual std::string toString() const;
	virtual JsonNode toJsonNode() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
	}
};

class DLL_LINKAGE IBonusBearer
{
private:
	static CSelector anaffectedByMoraleSelector;
	CCheckProxy anaffectedByMorale;
	static CSelector moraleSelector;
	CTotalsProxy moraleValue;
	static CSelector luckSelector;
	CTotalsProxy luckValue;

public:
	//new bonusing node interface
	// * selector is predicate that tests if HeroBonus matches our criteria
	// * root is node on which call was made (nullptr will be replaced with this)
	//interface
	IBonusBearer();
	virtual ~IBonusBearer() = default;
	virtual TConstBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = nullptr, const std::string &cachingStr = "") const = 0;
	int valOfBonuses(const CSelector &selector, const std::string &cachingStr = "") const;
	bool hasBonus(const CSelector &selector, const std::string &cachingStr = "") const;
	bool hasBonus(const CSelector &selector, const CSelector &limit, const std::string &cachingStr = "") const;
	TConstBonusListPtr getBonuses(const CSelector &selector, const CSelector &limit, const std::string &cachingStr = "") const;
	TConstBonusListPtr getBonuses(const CSelector &selector, const std::string &cachingStr = "") const;

	std::shared_ptr<const Bonus> getBonus(const CSelector &selector) const; //returns any bonus visible on node that matches (or nullptr if none matches)

	//legacy interface
	int valOfBonuses(Bonus::BonusType type, const CSelector &selector) const;
	int valOfBonuses(Bonus::BonusType type, int subtype = -1) const; //subtype -> subtype of bonus, if -1 then anyt;
	bool hasBonusOfType(Bonus::BonusType type, int subtype = -1) const;//determines if hero has a bonus of given type (and optionally subtype)
	bool hasBonusFrom(Bonus::BonusSource source, ui32 sourceID) const;

	//various hlp functions for non-trivial values
	//used for stacks and creatures only

	virtual int getMinDamage(bool ranged) const;
	virtual int getMaxDamage(bool ranged) const;
	virtual int getAttack(bool ranged) const;
	virtual int getDefense(bool ranged) const;

	int MoraleVal() const; //range [-3, +3]
	int LuckVal() const; //range [-3, +3]
	/**
	 Returns total value of all morale bonuses and sets bonusList as a pointer to the list of selected bonuses.
	 @param bonusList is the out param it's list of all selected bonuses
	 @return total value of all morale in the range [-3, +3] and 0 otherwise
	*/
	int MoraleValAndBonusList(TConstBonusListPtr & bonusList) const;
	int LuckValAndBonusList(TConstBonusListPtr & bonusList) const;

	ui32 MaxHealth() const; //get max HP of stack with all modifiers
	bool isLiving() const; //non-undead, non-non living or alive
	virtual si32 magicResistance() const;
	ui32 Speed(int turn = 0, bool useBind = false) const; //get speed of creature with all modificators

	int getPrimSkillLevel(PrimarySkill::PrimarySkill id) const;

	virtual int64_t getTreeVersion() const = 0;
};

class DLL_LINKAGE CBonusSystemNode : public virtual IBonusBearer, public boost::noncopyable
{
public:
	enum ENodeTypes
	{
		NONE = -1, 
		UNKNOWN, STACK_INSTANCE, STACK_BATTLE, SPECIALTY, ARTIFACT, CREATURE, ARTIFACT_INSTANCE, HERO, PLAYER, TEAM,
		TOWN_AND_VISITOR, BATTLE, COMMANDER, GLOBAL_EFFECTS, ALL_CREATURES, TOWN
	};
private:
	BonusList bonuses; //wielded bonuses (local or up-propagated here)
	BonusList exportedBonuses; //bonuses coming from this node (wielded or propagated away)

	TNodesVector parents; //parents -> we inherit bonuses from them, we may attach our bonuses to them
	TNodesVector children;

	ENodeTypes nodeType;
	std::string description;
	bool isHypotheticNode;

	static const bool cachingEnabled;
	mutable BonusList cachedBonuses;
	mutable int64_t cachedLast;
	static std::atomic<int64_t> treeChanged;

	// Setting a value to cachingStr before getting any bonuses caches the result for later requests.
	// This string needs to be unique, that's why it has to be setted in the following manner:
	// [property key]_[value] => only for selector
	mutable std::map<std::string, TBonusListPtr > cachedRequests;
	mutable boost::mutex sync;

	void getAllBonusesRec(BonusList &out, const CSelector & selector) const;
	TConstBonusListPtr getAllBonusesWithoutCaching(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = nullptr) const;
	std::shared_ptr<Bonus> getUpdatedBonus(const std::shared_ptr<Bonus> & b, const TUpdaterPtr & updater) const;

public:
	explicit CBonusSystemNode();
	explicit CBonusSystemNode(bool isHypotetic);
	explicit CBonusSystemNode(ENodeTypes NodeType);
	CBonusSystemNode(CBonusSystemNode && other) noexcept;
	virtual ~CBonusSystemNode();

	void limitBonuses(const BonusList &allBonuses, BonusList &out) const; //out will bo populed with bonuses that are not limited here
	TBonusListPtr limitBonuses(const BonusList &allBonuses) const; //same as above, returns out by val for convienence
	TConstBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = nullptr, const std::string &cachingStr = "") const override;
	void getParents(TCNodes &out) const;  //retrieves list of parent nodes (nodes to inherit bonuses from),
	std::shared_ptr<const Bonus> getBonusLocalFirst(const CSelector & selector) const;

	//non-const interface
	void getParents(TNodes &out);  //retrieves list of parent nodes (nodes to inherit bonuses from)

	void getRedParents(TNodes &out);  //retrieves list of red parent nodes (nodes bonuses propagate from)
	void getRedAncestors(TNodes &out);
	void getRedChildren(TNodes &out);
	void getAllParents(TCNodes & out) const;
	static PlayerColor retrieveNodeOwner(const CBonusSystemNode * node);
	std::shared_ptr<Bonus> getBonusLocalFirst(const CSelector & selector);

	void attachTo(CBonusSystemNode & parent);
	void detachFrom(CBonusSystemNode & parent);
	void detachFromAll();
	virtual void addNewBonus(const std::shared_ptr<Bonus>& b);
	void accumulateBonus(const std::shared_ptr<Bonus>& b); //add value of bonus with same type/subtype or create new

	void newChildAttached(CBonusSystemNode & child);
	void childDetached(CBonusSystemNode & child);
	void propagateBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & source);
	void unpropagateBonus(const std::shared_ptr<Bonus> & b);
	void removeBonus(const std::shared_ptr<Bonus>& b);
	void removeBonuses(const CSelector & selector);
	void removeBonusesRecursive(const CSelector & s);
	void newRedDescendant(CBonusSystemNode & descendant); //propagation needed
	void removedRedDescendant(CBonusSystemNode & descendant); //de-propagation needed

	bool isIndependentNode() const; //node is independent when it has no parents nor children
	bool actsAsBonusSourceOnly() const;
	///updates count of remaining turns and removes outdated bonuses by selector
	void reduceBonusDurations(const CSelector &s);
	virtual std::string bonusToString(const std::shared_ptr<Bonus>& bonus, bool description) const {return "";}; //description or bonus name
	virtual std::string nodeName() const;
	virtual std::string nodeShortInfo() const;
	bool isHypothetic() const { return isHypotheticNode; }

	void deserializationFix();
	void exportBonus(const std::shared_ptr<Bonus> & b);
	void exportBonuses();

	const BonusList &getBonusList() const;
	BonusList & getExportedBonusList();
	const BonusList & getExportedBonusList() const;
	CBonusSystemNode::ENodeTypes getNodeType() const;
	void setNodeType(CBonusSystemNode::ENodeTypes type);
	const TNodesVector &getParentNodes() const;
	const TNodesVector &getChildrenNodes() const;
	const std::string &getDescription() const;
	void setDescription(const std::string &description);

	static void treeHasChanged();

	int64_t getTreeVersion() const override;

	virtual PlayerColor getOwner() const
	{
		return PlayerColor::NEUTRAL;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
//		h & bonuses;
		h & nodeType;
		h & exportedBonuses;
		h & description;
		BONUS_TREE_DESERIALIZATION_FIX
		//h & parents & children;
	}

	friend class CBonusProxy;
};

class DLL_LINKAGE IPropagator
{
public:
	virtual ~IPropagator() = default;
	virtual bool shouldBeAttached(CBonusSystemNode *dest);
	virtual CBonusSystemNode::ENodeTypes getPropagatorType() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{}
};

class DLL_LINKAGE CPropagatorNodeType : public IPropagator
{
	CBonusSystemNode::ENodeTypes nodeType;

public:
	CPropagatorNodeType(CBonusSystemNode::ENodeTypes NodeType = CBonusSystemNode::ENodeTypes::UNKNOWN);
	bool shouldBeAttached(CBonusSystemNode *dest) override;
	CBonusSystemNode::ENodeTypes getPropagatorType() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & nodeType;
	}
};

template<typename T>
class CSelectFieldEqual
{
	T Bonus::*ptr;

public:
	CSelectFieldEqual(T Bonus::*Ptr)
		: ptr(Ptr)
	{
	}

	CSelector operator()(const T &valueToCompareAgainst) const
	{
		auto ptr2 = ptr; //We need a COPY because we don't want to reference this (might be outlived by lambda)
		return [ptr2, valueToCompareAgainst](const Bonus *bonus)
		{
			return bonus->*ptr2 == valueToCompareAgainst;
		};
	}
};

template<typename T> //can be same, needed for subtype field
class CSelectFieldEqualOrEvery
{
	T Bonus::*ptr;
	T val;
public:
	CSelectFieldEqualOrEvery(T Bonus::*Ptr, const T &Val)
		: ptr(Ptr), val(Val)
	{
	}
	bool operator()(const Bonus *bonus) const
	{
		return (bonus->*ptr == val) || (bonus->*ptr == static_cast<T>(Bonus::EVERY_TYPE));
	}
	CSelectFieldEqualOrEvery& operator()(const T &setVal)
	{
		val = setVal;
		return *this;
	}
};

class DLL_LINKAGE CWillLastTurns
{
public:
	int turnsRequested;

	bool operator()(const Bonus *bonus) const
	{
		return turnsRequested <= 0					//every present effect will last zero (or "less") turns
			|| !Bonus::NTurns(bonus) //so do every not expriing after N-turns effect
			|| bonus->turnsRemain > turnsRequested;
	}
	CWillLastTurns& operator()(const int &setVal)
	{
		turnsRequested = setVal;
		return *this;
	}
};

class DLL_LINKAGE CWillLastDays
{
public:
	int daysRequested;

	bool operator()(const Bonus *bonus) const
	{
		if(daysRequested <= 0 || Bonus::Permanent(bonus) || Bonus::OneBattle(bonus))
			return true;
		else if(Bonus::OneDay(bonus))
			return false;
		else if(Bonus::NDays(bonus) || Bonus::OneWeek(bonus))
		{
			return bonus->turnsRemain > daysRequested;
		}

		return false; // TODO: ONE_WEEK need support for turnsRemain, but for now we'll exclude all unhandled durations
	}
	CWillLastDays& operator()(const int &setVal)
	{
		daysRequested = setVal;
		return *this;
	}
};

class DLL_LINKAGE AggregateLimiter : public ILimiter
{
protected:
	std::vector<TLimiterPtr> limiters;
	virtual const std::string & getAggregator() const = 0;
	AggregateLimiter(std::vector<TLimiterPtr> limiters = {});
public:
	void add(const TLimiterPtr & limiter);
	JsonNode toJsonNode() const override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & limiters;
	}
};

class DLL_LINKAGE AllOfLimiter : public AggregateLimiter
{
protected:
	const std::string & getAggregator() const override;
public:
	AllOfLimiter(std::vector<TLimiterPtr> limiters = {});
	static const std::string aggregator;
	EDecision limit(const BonusLimitationContext & context) const override;
};

class DLL_LINKAGE AnyOfLimiter : public AggregateLimiter
{
protected:
	const std::string & getAggregator() const override;
public:
	AnyOfLimiter(std::vector<TLimiterPtr> limiters = {});
	static const std::string aggregator;
	EDecision limit(const BonusLimitationContext & context) const override;
};

class DLL_LINKAGE NoneOfLimiter : public AggregateLimiter
{
protected:
	const std::string & getAggregator() const override;
public:
	NoneOfLimiter(std::vector<TLimiterPtr> limiters = {});
	static const std::string aggregator;
	EDecision limit(const BonusLimitationContext & context) const override;
};

class DLL_LINKAGE CCreatureTypeLimiter : public ILimiter //affect only stacks of given creature (and optionally it's upgrades)
{
public:
	const CCreature * creature = nullptr;
	bool includeUpgrades = false;

	CCreatureTypeLimiter() = default;
	CCreatureTypeLimiter(const CCreature & creature_, bool IncludeUpgrades);
	void setCreature(const CreatureID & id);

	EDecision limit(const BonusLimitationContext &context) const override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & creature;
		h & includeUpgrades;
	}
};

class DLL_LINKAGE HasAnotherBonusLimiter : public ILimiter //applies only to nodes that have another bonus working
{
public:
	Bonus::BonusType type;
	TBonusSubtype subtype;
	Bonus::BonusSource source;
	si32 sid;
	bool isSubtypeRelevant; //check for subtype only if this is true
	bool isSourceRelevant; //check for bonus source only if this is true
	bool isSourceIDRelevant; //check for bonus source only if this is true

	HasAnotherBonusLimiter(Bonus::BonusType bonus = Bonus::NONE);
	HasAnotherBonusLimiter(Bonus::BonusType bonus, TBonusSubtype _subtype);
	HasAnotherBonusLimiter(Bonus::BonusType bonus, Bonus::BonusSource src);
	HasAnotherBonusLimiter(Bonus::BonusType bonus, TBonusSubtype _subtype, Bonus::BonusSource src);

	EDecision limit(const BonusLimitationContext &context) const override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & type;
		h & subtype;
		h & isSubtypeRelevant;
		h & source;
		h & isSourceRelevant;
		h & sid;
		h & isSourceIDRelevant;
	}
};

class DLL_LINKAGE CreatureTerrainLimiter : public ILimiter //applies only to creatures that are on specified terrain, default native terrain
{
public:
	TerrainId terrainType;
	CreatureTerrainLimiter();
	CreatureTerrainLimiter(TerrainId terrain);

	EDecision limit(const BonusLimitationContext &context) const override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & terrainType;
	}
};

class DLL_LINKAGE CreatureLevelLimiter : public ILimiter //applies only to creatures of given faction
{
public:
	uint32_t minLevel;
	uint32_t maxLevel;
	//accept all levels by default, accept creatures of minLevel <= creature->getLevel() < maxLevel
	CreatureLevelLimiter(uint32_t minLevel = std::numeric_limits<uint32_t>::min(), uint32_t maxLevel = std::numeric_limits<uint32_t>::max());

	EDecision limit(const BonusLimitationContext &context) const override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & minLevel;
		h & maxLevel;
	}
};

class DLL_LINKAGE FactionLimiter : public ILimiter //applies only to creatures of given faction
{
public:
	FactionID faction;
	FactionLimiter(FactionID faction = FactionID::DEFAULT);

	EDecision limit(const BonusLimitationContext &context) const override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & faction;
	}
};

class DLL_LINKAGE CreatureAlignmentLimiter : public ILimiter //applies only to creatures of given alignment
{
public:
	EAlignment alignment;
	CreatureAlignmentLimiter(EAlignment Alignment = EAlignment::NEUTRAL);

	EDecision limit(const BonusLimitationContext &context) const override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & alignment;
	}
};

class DLL_LINKAGE OppositeSideLimiter : public ILimiter //applies only to creatures of enemy army during combat
{
public:
	PlayerColor owner;
	OppositeSideLimiter(PlayerColor Owner = PlayerColor::CANNOT_DETERMINE);

	EDecision limit(const BonusLimitationContext &context) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & owner;
	}
};

class DLL_LINKAGE RankRangeLimiter : public ILimiter //applies to creatures with min <= Rank <= max
{
public:
	ui8 minRank, maxRank;

	RankRangeLimiter();
	RankRangeLimiter(ui8 Min, ui8 Max = 255);
	EDecision limit(const BonusLimitationContext &context) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & minRank;
		h & maxRank;
	}
};

class DLL_LINKAGE UnitOnHexLimiter : public ILimiter //works only on selected hexes
{
public:
	std::set<BattleHex> applicableHexes;

	UnitOnHexLimiter(const std::set<BattleHex> & applicableHexes = {});
	EDecision limit(const BonusLimitationContext &context) const override;
	JsonNode toJsonNode() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & applicableHexes;
	}
};

namespace Selector
{
	extern DLL_LINKAGE CSelectFieldEqual<Bonus::BonusType> & type();
	extern DLL_LINKAGE CSelectFieldEqual<TBonusSubtype> & subtype();
	extern DLL_LINKAGE CSelectFieldEqual<CAddInfo> & info();
	extern DLL_LINKAGE CSelectFieldEqual<Bonus::BonusSource> & sourceType();
	extern DLL_LINKAGE CSelectFieldEqual<Bonus::BonusSource> & targetSourceType();
	extern DLL_LINKAGE CSelectFieldEqual<Bonus::LimitEffect> & effectRange();
	extern DLL_LINKAGE CWillLastTurns turns;
	extern DLL_LINKAGE CWillLastDays days;

	CSelector DLL_LINKAGE typeSubtype(Bonus::BonusType Type, TBonusSubtype Subtype);
	CSelector DLL_LINKAGE typeSubtypeInfo(Bonus::BonusType type, TBonusSubtype subtype, const CAddInfo & info);
	CSelector DLL_LINKAGE source(Bonus::BonusSource source, ui32 sourceID);
	CSelector DLL_LINKAGE sourceTypeSel(Bonus::BonusSource source);
	CSelector DLL_LINKAGE valueType(Bonus::ValueType valType);

	/**
	 * Selects all bonuses
	 * Usage example: Selector::all.And(<functor>).And(<functor>)...)
	 */
	extern DLL_LINKAGE CSelector all;

	/**
	 * Selects nothing
	 * Usage example: Selector::none.Or(<functor>).Or(<functor>)...)
	 */
	extern DLL_LINKAGE CSelector none;
}

extern DLL_LINKAGE const std::map<std::string, Bonus::BonusType> bonusNameMap;
extern DLL_LINKAGE const std::map<std::string, Bonus::ValueType> bonusValueMap;
extern DLL_LINKAGE const std::map<std::string, Bonus::BonusSource> bonusSourceMap;
extern DLL_LINKAGE const std::map<std::string, ui16> bonusDurationMap;
extern DLL_LINKAGE const std::map<std::string, Bonus::LimitEffect> bonusLimitEffect;
extern DLL_LINKAGE const std::map<std::string, TLimiterPtr> bonusLimiterMap;
extern DLL_LINKAGE const std::map<std::string, TPropagatorPtr> bonusPropagatorMap;
extern DLL_LINKAGE const std::map<std::string, TUpdaterPtr> bonusUpdaterMap;
extern DLL_LINKAGE const std::set<std::string> deprecatedBonusSet;

// BonusList template that requires full interface of CBonusSystemNode
template <class InputIterator>
void BonusList::insert(const int position, InputIterator first, InputIterator last)
{
	bonuses.insert(bonuses.begin() + position, first, last);
	changed();
}

// observers for updating bonuses based on certain events (e.g. hero gaining level)

class DLL_LINKAGE IUpdater
{
public:
	virtual ~IUpdater() = default;

	virtual std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const;
	virtual std::string toString() const;
	virtual JsonNode toJsonNode() const;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
	}
};

class DLL_LINKAGE GrowsWithLevelUpdater : public IUpdater
{
public:
	int valPer20 = 0;
	int stepSize = 1;

	GrowsWithLevelUpdater() = default;
	GrowsWithLevelUpdater(int valPer20, int stepSize = 1);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<IUpdater &>(*this);
		h & valPer20;
		h & stepSize;
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const override;
	virtual std::string toString() const override;
	virtual JsonNode toJsonNode() const override;
};

class DLL_LINKAGE TimesHeroLevelUpdater : public IUpdater
{
public:
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<IUpdater &>(*this);
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const override;
	virtual std::string toString() const override;
	virtual JsonNode toJsonNode() const override;
};

class DLL_LINKAGE TimesStackLevelUpdater : public IUpdater
{
public:
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<IUpdater &>(*this);
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const override;
	virtual std::string toString() const override;
	virtual JsonNode toJsonNode() const override;
};

class DLL_LINKAGE ArmyMovementUpdater : public IUpdater
{
public:
	si32 base;
	si32 divider;
	si32 multiplier;
	si32 max;
	ArmyMovementUpdater();
	ArmyMovementUpdater(int base, int divider, int multiplier, int max);
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<IUpdater &>(*this);
		h & base;
		h & divider;
		h & multiplier;
		h & max;
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const override;
	virtual std::string toString() const override;
	virtual JsonNode toJsonNode() const override;
};

class DLL_LINKAGE OwnerUpdater : public IUpdater
{
public:
	template <typename Handler> void serialize(Handler& h, const int version)
	{
		h & static_cast<IUpdater &>(*this);
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus>& b, const CBonusSystemNode& context) const override;
	virtual std::string toString() const override;
	virtual JsonNode toJsonNode() const override;
};

VCMI_LIB_NAMESPACE_END
