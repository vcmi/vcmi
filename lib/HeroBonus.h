#pragma once

#include "GameConstants.h"

/*
 * HeroBonus.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CCreature;
class CSpell;
struct Bonus;
class CBonusSystemNode;
class ILimiter;
class IPropagator;
class BonusList;

typedef std::shared_ptr<BonusList> TBonusListPtr;
typedef std::shared_ptr<ILimiter> TLimiterPtr;
typedef std::shared_ptr<IPropagator> TPropagatorPtr;
typedef std::set<CBonusSystemNode*> TNodes;
typedef std::set<const CBonusSystemNode*> TCNodes;
typedef std::vector<CBonusSystemNode *> TNodesVector;

class CSelector : std::function<bool(const Bonus*)>
{
	typedef std::function<bool(const Bonus*)> TBase;
public:
	CSelector() {}
	template<typename T>
	CSelector(const T &t,	//SFINAE trick -> include this c-tor in overload resolution only if parameter is class 
							//(includes functors, lambdas) or function. Without that VC is going mad about ambiguities.
		typename std::enable_if < boost::mpl::or_ < std::is_class<T>, std::is_function<T >> ::value>::type *dummy = nullptr)
		: TBase(t)
	{}

	CSelector(std::nullptr_t)
	{}
	//CSelector(std::function<bool(const Bonus*)> f) : std::function<bool(const Bonus*)>(std::move(f)) {}

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

	bool operator()(const Bonus *b) const
	{
		return TBase::operator()(b);
	}

	operator bool() const
	{
		return !!static_cast<const TBase&>(*this);
	}
};



#define BONUS_TREE_DESERIALIZATION_FIX if(!h.saving && h.smartPointerSerialization) deserializationFix();

#define BONUS_LIST										\
	BONUS_NAME(NONE) 									\
	BONUS_NAME(LEVEL_COUNTER) /* for commander artifacts*/ \
	BONUS_NAME(MOVEMENT) /*both water/land*/			\
	BONUS_NAME(LAND_MOVEMENT) \
	BONUS_NAME(SEA_MOVEMENT) \
	BONUS_NAME(MORALE) \
	BONUS_NAME(LUCK) \
	BONUS_NAME(PRIMARY_SKILL) /*uses subtype to pick skill; additional info if set: 1 - only melee, 2 - only distance*/  \
	BONUS_NAME(SIGHT_RADIOUS) \
	BONUS_NAME(MANA_REGENERATION) /*points per turn apart from normal (1 + mysticism)*/  \
	BONUS_NAME(FULL_MANA_REGENERATION) /*all mana points are replenished every day*/  \
	BONUS_NAME(NONEVIL_ALIGNMENT_MIX) /*good and neutral creatures can be mixed without morale penalty*/  \
	BONUS_NAME(SECONDARY_SKILL_PREMY) /*%*/  \
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
	BONUS_NAME(BLOCK_MORALE) \
	BONUS_NAME(BLOCK_LUCK) \
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
	BONUS_NAME(IMPROVED_NECROMANCY) /*allows Necropolis units other than skeletons to be raised by necromancy*/ \
	BONUS_NAME(CREATURE_GROWTH_PERCENT) /*increases growth of all units in all towns, val - percentage*/ \
	BONUS_NAME(FREE_SHIP_BOARDING) /*movement points preserved with ship boarding and landing*/  \
	BONUS_NAME(NO_TYPE)									\
	BONUS_NAME(FLYING)									\
	BONUS_NAME(SHOOTER)									\
	BONUS_NAME(CHARGE_IMMUNITY)							\
	BONUS_NAME(ADDITIONAL_ATTACK)						\
	BONUS_NAME(UNLIMITED_RETALIATIONS)					\
	BONUS_NAME(NO_MELEE_PENALTY)						\
	BONUS_NAME(JOUSTING) /*for champions*/				\
	BONUS_NAME(HATE) /*eg. angels hate devils, subtype - ID of hated creature, val - damage bonus percent */ \
	BONUS_NAME(KING1)									\
	BONUS_NAME(KING2)									\
	BONUS_NAME(KING3)									\
	BONUS_NAME(MAGIC_RESISTANCE) /*in % (value)*/		\
	BONUS_NAME(CHANGES_SPELL_COST_FOR_ALLY) /*in mana points (value) , eg. mage*/ \
	BONUS_NAME(CHANGES_SPELL_COST_FOR_ENEMY) /*in mana points (value) , eg. pegasus */ \
	BONUS_NAME(SPELL_AFTER_ATTACK) /* subtype - spell id, value - chance %, additional info % 1000 - level, (additional info)/1000 -> [0 - all attacks, 1 - shot only, 2 - melee only*/ \
	BONUS_NAME(SPELL_BEFORE_ATTACK) /* subtype - spell id, value - chance %, additional info % 1000 - level, (additional info)/1000 -> [0 - all attacks, 1 - shot only, 2 - melee only*/ \
	BONUS_NAME(SPELL_RESISTANCE_AURA) /*eg. unicorns, value - resistance bonus in % for adjacent creatures*/ \
	BONUS_NAME(LEVEL_SPELL_IMMUNITY) /*creature is immune to all spell with level below or equal to value of this bonus*/ \
	BONUS_NAME(BLOCK_MAGIC_ABOVE) /*blocks casting spells of the level > value */ \
	BONUS_NAME(BLOCK_ALL_MAGIC) /*blocks casting spells*/ \
	BONUS_NAME(TWO_HEX_ATTACK_BREATH) /*eg. dragons*/	\
	BONUS_NAME(SPELL_DAMAGE_REDUCTION) /*eg. golems; value - reduction in %, subtype - spell school; -1 - all, 0 - air, 1 - fire, 2 - water, 3 - earth*/ \
	BONUS_NAME(NO_WALL_PENALTY)							\
	BONUS_NAME(NON_LIVING) /*eg. gargoyle*/				\
	BONUS_NAME(RANDOM_SPELLCASTER) /*eg. master genie, val - level*/ \
	BONUS_NAME(BLOCKS_RETALIATION) /*eg. naga*/			\
	BONUS_NAME(SPELL_IMMUNITY) /*subid - spell id*/		\
	BONUS_NAME(MANA_CHANNELING) /*value in %, eg. familiar*/ \
	BONUS_NAME(SPELL_LIKE_ATTACK) /*subtype - spell, value - spell level; range is taken from spell, but damage from creature; eg. magog*/ \
	BONUS_NAME(THREE_HEADED_ATTACK) /*eg. cerberus*/	\
	BONUS_NAME(DAEMON_SUMMONING) /*pit lord, subtype - type of creatures, val - hp per unit*/			\
	BONUS_NAME(FIRE_IMMUNITY)	/*subtype 0 - all, 1 - all except positive, 2 - only damage spells*/						\
	BONUS_NAME(WATER_IMMUNITY)							\
	BONUS_NAME(EARTH_IMMUNITY)							\
	BONUS_NAME(AIR_IMMUNITY)							\
	BONUS_NAME(MIND_IMMUNITY)							\
	BONUS_NAME(FIRE_SHIELD)								\
	BONUS_NAME(UNDEAD)									\
	BONUS_NAME(HP_REGENERATION) /*creature regenerates val HP every new round*/					\
	BONUS_NAME(FULL_HP_REGENERATION) /*first creature regenerates all HP every new round; subtype 0 - animation 4 (trolllike), 1 - animation 47 (wightlike)*/		\
	BONUS_NAME(MANA_DRAIN) /*value - spell points per turn*/ \
	BONUS_NAME(LIFE_DRAIN)								\
	BONUS_NAME(DOUBLE_DAMAGE_CHANCE) /*value in %, eg. dread knight*/ \
	BONUS_NAME(RETURN_AFTER_STRIKE)						\
	BONUS_NAME(SELF_MORALE) /*eg. minotaur*/			\
	BONUS_NAME(SPELLCASTER) /*subtype - spell id, value - level of school, additional info - weighted chance. use SPECIFIC_SPELL_POWER, CREATURE_SPELL_POWER or CREATURE_ENCHANT_POWER for calculating the power*/ \
	BONUS_NAME(CATAPULT)								\
	BONUS_NAME(ENEMY_DEFENCE_REDUCTION) /*in % (value) eg. behemots*/ \
	BONUS_NAME(GENERAL_DAMAGE_REDUCTION) /* shield / air shield effect */ \
	BONUS_NAME(GENERAL_ATTACK_REDUCTION) /*eg. while stoned or blinded - in %,// subtype not used, use ONLY_MELEE_FIGHT / DISTANCE_FIGHT*/ \
	BONUS_NAME(DEFENSIVE_STANCE) /* val - bonus to defense while defending */ \
	BONUS_NAME(ATTACKS_ALL_ADJACENT) /*eg. hydra*/		\
	BONUS_NAME(MORE_DAMAGE_FROM_SPELL) /*value - damage increase in %, subtype - spell id*/ \
	BONUS_NAME(FEAR)									\
	BONUS_NAME(FEARLESS)								\
	BONUS_NAME(NO_DISTANCE_PENALTY)						\
	BONUS_NAME(SELF_LUCK) /*halfling*/					\
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
	BONUS_NAME(SPECIAL_SECONDARY_SKILL) /*val = id, additionalInfo = value per level in percent*/ \
	BONUS_NAME(SPECIAL_SPELL_LEV) /*val = id, additionalInfo = value per level in percent*/\
	BONUS_NAME(SPELL_DAMAGE) /*val = value*/\
	BONUS_NAME(SPECIFIC_SPELL_DAMAGE) /*subtype = id of spell, val = value*/\
	BONUS_NAME(SPECIAL_BLESS_DAMAGE) /*val = spell (bless), additionalInfo = value per level in percent*/\
	BONUS_NAME(MAXED_SPELL) /*val = id*/\
	BONUS_NAME(SPECIAL_PECULIAR_ENCHANT) /*blesses and curses with id = val dependent on unit's level, subtype = 0 or 1 for Coronius*/\
	BONUS_NAME(SPECIAL_UPGRADE) /*val = base, additionalInfo = target */\
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
	BONUS_SOURCE(OTHER) /*used for defensive stance and default value of spell level limit*/

#define BONUS_VALUE_LIST \
	BONUS_VALUE(ADDITIVE_VALUE)\
	BONUS_VALUE(BASE_NUMBER)\
	BONUS_VALUE(PERCENT_TO_ALL)\
	BONUS_VALUE(PERCENT_TO_BASE)\
	BONUS_VALUE(INDEPENDENT_MAX) /*used for SPELL bonus*/ \
	BONUS_VALUE(INDEPENDENT_MIN) //used for SECONDARY_SKILL_PREMY bonus

/// Struct for handling bonuses of several types. Can be transferred to any hero
struct DLL_LINKAGE Bonus
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
		UNITL_BEING_ATTACKED = 64,/*removed after attack and counterattacks are performed*/
		UNTIL_ATTACK = 128, /*removed after attack and counterattacks are performed*/
		STACK_GETS_TURN = 256, /*removed when stack gets its turn - used for defensive stance*/
		COMMANDER_KILLED = 512
	};
	enum BonusSource
	{
#define BONUS_SOURCE(x) x,
		BONUS_SOURCE_LIST
#undef BONUS_SOURCE
	};

	enum LimitEffect
	{
		NO_LIMIT = 0,
		ONLY_DISTANCE_FIGHT=1, ONLY_MELEE_FIGHT, //used to mark bonuses for attack/defense primary skills from spells like Precision (distance only)
		ONLY_ENEMY_ARMY
	};

	enum ValueType
	{
#define BONUS_VALUE(x) x,
		BONUS_VALUE_LIST
#undef BONUS_VALUE
	};

	ui16 duration; //uses BonusDuration values
	si16 turnsRemain; //used if duration is N_TURNS, N_DAYS or ONE_WEEK

	BonusType type; //uses BonusType values - says to what is this bonus - 1 byte
	TBonusSubtype subtype; //-1 if not applicable - 4 bytes

	BonusSource source;//source type" uses BonusSource values - what gave that bonus
	si32 val;
	ui32 sid; //source id: id of object/artifact/spell
	ValueType valType;

	si32 additionalInfo;
	LimitEffect effectRange; //if not NO_LIMIT, bonus will be omitted by default

	TLimiterPtr limiter;
	TPropagatorPtr propagator;

	std::string description;

	Bonus(ui16 Dur, BonusType Type, BonusSource Src, si32 Val, ui32 ID, std::string Desc, si32 Subtype=-1);
	Bonus(ui16 Dur, BonusType Type, BonusSource Src, si32 Val, ui32 ID, si32 Subtype=-1, ValueType ValType = ADDITIVE_VALUE);
	Bonus();
	~Bonus();

// 	//comparison
// 	bool operator==(const HeroBonus &other)
// 	{
// 		return &other == this;
// 		//TODO: what is best logic for that?
// 	}
// 	bool operator<(const HeroBonus &other)
// 	{
// 		return &other < this;
// 		//TODO: what is best logic for that?
// 	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & duration & type & subtype & source & val & sid & description & additionalInfo & turnsRemain & valType & effectRange & limiter & propagator;
	}

	static bool compareByAdditionalInfo(const Bonus *a, const Bonus *b)
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
		return hb->duration & Bonus::UNITL_BEING_ATTACKED;
	}
	static bool UntilCommanderKilled(const Bonus *hb)
	{
		return hb->duration & Bonus::COMMANDER_KILLED;
	}
	static bool IsFrom(const Bonus &hb, ui8 source, ui32 id) //if id==0xffffff then id doesn't matter
	{
		return hb.source==source && (id==0xffffff  ||  hb.sid==id);
	}
	inline bool operator == (const BonusType & cf) const
	{
		return type == cf;
	}
	inline void ChangeBonusVal (const ui32 newVal)
	{
		val = newVal;
	}
	inline void operator += (const ui32 Val) //no return
	{
		val += Val;
	}
	const CSpell * sourceSpell() const;

	std::string Description() const;

	Bonus *addLimiter(TLimiterPtr Limiter); //returns this for convenient chain-calls
	Bonus *addPropagator(TPropagatorPtr Propagator); //returns this for convenient chain-calls
};

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const Bonus &bonus);


class DLL_LINKAGE BonusList
{
private:
	typedef std::vector<Bonus*> TInternalContainer;

	TInternalContainer bonuses;
	bool belongsToTree;


public:
	typedef TInternalContainer::const_reference const_reference;
	typedef TInternalContainer::value_type value_type;

	typedef TInternalContainer::const_iterator const_iterator;
	typedef TInternalContainer::iterator iterator;

	BonusList(bool BelongsToTree = false);
	BonusList(const BonusList &bonusList);
	BonusList& operator=(const BonusList &bonusList);

	// wrapper functions of the STL vector container
	std::vector<Bonus*>::size_type size() const { return bonuses.size(); }
	void push_back(Bonus* const &x);
	std::vector<Bonus*>::iterator erase (const int position);
	void clear();
	bool empty() const { return bonuses.empty(); }
	void resize(std::vector<Bonus*>::size_type sz, Bonus* c = nullptr );
	void insert(std::vector<Bonus*>::iterator position, std::vector<Bonus*>::size_type n, Bonus* const &x);
	Bonus *const &operator[] (std::vector<Bonus*>::size_type n) { return bonuses[n]; }
	Bonus *const &operator[] (std::vector<Bonus*>::size_type n) const { return bonuses[n]; }
	Bonus *const &back() { return bonuses.back(); }
	Bonus *const &front() { return bonuses.front(); }
	Bonus *const &back() const { return bonuses.back(); }
	Bonus *const &front() const { return bonuses.front(); }

	// There should be no non-const access to provide solid,robust bonus caching
	std::vector<Bonus*>::const_iterator begin() const { return bonuses.begin(); }
	std::vector<Bonus*>::const_iterator end() const { return bonuses.end(); }
	std::vector<Bonus*>::size_type operator-=(Bonus* const &i);

	// BonusList functions
	int totalValue() const; //subtype -> subtype of bonus, if -1 then any
	void getBonuses(BonusList &out, const CSelector &selector, const CSelector &limit) const;
	void getAllBonuses(BonusList &out) const;

	void getBonuses(BonusList & out, const CSelector &selector) const;

	//special find functions
	Bonus *getFirst(const CSelector &select);
	const Bonus *getFirst(const CSelector &select) const;
	int valOfBonuses(const CSelector &select) const;

	//void limit(const CBonusSystemNode &node); //erases bonuses using limitor
	void eliminateDuplicates();

	// remove_if implementation for STL vector types
	template <class Predicate>
	void remove_if(Predicate pred)
	{
		BonusList newList;
		for (ui32 i = 0; i < bonuses.size(); i++)
		{
			Bonus *b = bonuses[i];
			if (!pred(b))
				newList.push_back(b);
		}
		bonuses.clear();
		bonuses.resize(newList.size());
		std::copy(newList.begin(), newList.end(), bonuses.begin());
	}

	template <class InputIterator>
	void insert(const int position, InputIterator first, InputIterator last);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<std::vector<Bonus*>&>(bonuses);
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

	//friend inline std::vector<Bonus*>::iterator range_begin(BonusList & x);
	//friend inline std::vector<Bonus*>::iterator range_end(BonusList & x);
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

inline BonusList::const_iterator range_begin(BonusList const &x)
{
	return x.begin();
}

inline BonusList::const_iterator range_end(BonusList const &x)
{
	return x.end();
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const BonusList &bonusList);

class DLL_LINKAGE IPropagator
{
public:
	virtual ~IPropagator();
	virtual bool shouldBeAttached(CBonusSystemNode *dest);
	//virtual CBonusSystemNode *getDestNode(CBonusSystemNode *source, CBonusSystemNode *redParent, CBonusSystemNode *redChild); //called when red relation between parent-childrem is established / removed

	template <typename Handler> void serialize(Handler &h, const int version)
	{}
};

class DLL_LINKAGE CPropagatorNodeType : public IPropagator
{
	int nodeType; //CBonusSystemNode::ENodeTypes
public:
	CPropagatorNodeType();
	CPropagatorNodeType(int NodeType);
	bool shouldBeAttached(CBonusSystemNode *dest) override;
	//CBonusSystemNode *getDestNode(CBonusSystemNode *source, CBonusSystemNode *redParent, CBonusSystemNode *redChild) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & nodeType;
	}
};

struct BonusLimitationContext
{
	const Bonus *b;
	const CBonusSystemNode &node;
	const BonusList &alreadyAccepted;
};

class DLL_LINKAGE ILimiter
{
public:
	enum EDecision {ACCEPT, DISCARD, NOT_SURE};

	virtual ~ILimiter();

	virtual int limit(const BonusLimitationContext &context) const; //0 - accept bonus; 1 - drop bonus; 2 - delay (drops eventually)

	template <typename Handler> void serialize(Handler &h, const int version)
	{
	}
};

class DLL_LINKAGE IBonusBearer
{
public:
	//new bonusing node interface
	// * selector is predicate that tests if HeroBonus matches our criteria
	// * root is node on which call was made (nullptr will be replaced with this)
	//interface
	virtual const TBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = nullptr, const std::string &cachingStr = "") const = 0;
	int getBonusesCount(const CSelector &selector, const std::string &cachingStr = "") const;
	int valOfBonuses(const CSelector &selector, const std::string &cachingStr = "") const;
	bool hasBonus(const CSelector &selector, const std::string &cachingStr = "") const;
	const TBonusListPtr getBonuses(const CSelector &selector, const CSelector &limit, const std::string &cachingStr = "") const;
	const TBonusListPtr getBonuses(const CSelector &selector, const std::string &cachingStr = "") const;

	const TBonusListPtr getAllBonuses() const;
	const Bonus *getBonus(const CSelector &selector) const; //returns any bonus visible on node that matches (or nullptr if none matches)

	//legacy interface
	int valOfBonuses(Bonus::BonusType type, const CSelector &selector) const;
	int valOfBonuses(Bonus::BonusType type, int subtype = -1) const; //subtype -> subtype of bonus, if -1 then anyt;
	bool hasBonusOfType(Bonus::BonusType type, int subtype = -1) const;//determines if hero has a bonus of given type (and optionally subtype)
	bool hasBonusFrom(Bonus::BonusSource source, ui32 sourceID) const;
	int getBonusesCount(Bonus::BonusSource from, int id) const;

	//various hlp functions for non-trivial values
	ui32 getMinDamage() const; //used for stacks and creatures only
	ui32 getMaxDamage() const;
	int MoraleVal() const; //range [-3, +3]
	int LuckVal() const; //range [-3, +3]
	si32 Attack() const; //get attack of stack with all modificators
	si32 Defense(bool withFrenzy = true) const; //get defense of stack with all modificators
	ui32 MaxHealth() const; //get max HP of stack with all modifiers
	bool isLiving() const; //non-undead, non-non living or alive
	virtual si32 magicResistance() const;
	ui32 Speed(int turn = 0, bool useBind = false) const; //get speed of creature with all modificators
	const Bonus * getEffect(ui16 id, int turn = 0) const; //effect id (SP)
	ui8 howManyEffectsSet(ui16 id) const; //returns amount of effects with given id set for this stack

	si32 manaLimit() const; //maximum mana value for this hero (basically 10*knowledge)
	int getPrimSkillLevel(PrimarySkill::PrimarySkill id) const;
	const TBonusListPtr getSpellBonuses() const;
};

class DLL_LINKAGE CBonusSystemNode : public IBonusBearer
{
public:
	enum ENodeTypes
	{
		UNKNOWN, STACK_INSTANCE, STACK_BATTLE, SPECIALTY, ARTIFACT, CREATURE, ARTIFACT_INSTANCE, HERO, PLAYER, TEAM,
		TOWN_AND_VISITOR, BATTLE, COMMANDER, GLOBAL_EFFECTS
	};
private:
	BonusList bonuses; //wielded bonuses (local or up-propagated here)
	BonusList exportedBonuses; //bonuses coming from this node (wielded or propagated away)

	TNodesVector parents; //parents -> we inherit bonuses from them, we may attach our bonuses to them
	TNodesVector children;

	ENodeTypes nodeType;
	std::string description;

	static const bool cachingEnabled;
	mutable BonusList cachedBonuses;
	mutable int cachedLast;
	static int treeChanged;

	// Setting a value to cachingStr before getting any bonuses caches the result for later requests.
	// This string needs to be unique, that's why it has to be setted in the following manner:
	// [property key]_[value] => only for selector
	mutable std::map<std::string, TBonusListPtr > cachedRequests;

	void getBonusesRec(BonusList &out, const CSelector &selector, const CSelector &limit) const;
	void getAllBonusesRec(BonusList &out) const;
	const TBonusListPtr getAllBonusesWithoutCaching(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = nullptr) const;

public:

	explicit CBonusSystemNode();
	virtual ~CBonusSystemNode();

	void limitBonuses(const BonusList &allBonuses, BonusList &out) const; //out will bo populed with bonuses that are not limited here
	TBonusListPtr limitBonuses(const BonusList &allBonuses) const; //same as above, returns out by val for convienence
	const TBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = nullptr, const std::string &cachingStr = "") const override;
	void getParents(TCNodes &out) const;  //retrieves list of parent nodes (nodes to inherit bonuses from),
	const Bonus *getBonusLocalFirst(const CSelector &selector) const;

	//non-const interface
	void getParents(TNodes &out);  //retrieves list of parent nodes (nodes to inherit bonuses from)
	void getRedParents(TNodes &out);  //retrieves list of red parent nodes (nodes bonuses propagate from)
	void getRedAncestors(TNodes &out);
	void getRedChildren(TNodes &out);
	void getRedDescendants(TNodes &out);
	Bonus *getBonusLocalFirst(const CSelector &selector);

	void attachTo(CBonusSystemNode *parent);
	void detachFrom(CBonusSystemNode *parent);
	void detachFromAll();
	virtual void addNewBonus(Bonus *b); //b will be deleted with destruction of node
	void accumulateBonus(Bonus &b); //add value of bonus with same type/subtype or create new

	void newChildAttached(CBonusSystemNode *child);
	void childDetached(CBonusSystemNode *child);
	void propagateBonus(Bonus * b);
	void unpropagateBonus(Bonus * b);
	//void addNewBonus(const Bonus &b); //b will copied
	void removeBonus(Bonus *b);
	void newRedDescendant(CBonusSystemNode *descendant); //propagation needed
	void removedRedDescendant(CBonusSystemNode *descendant); //de-propagation needed
	void battleTurnPassed(); //updates count of remaining turns and removed outdated bonuses

	bool isIndependentNode() const; //node is independent when it has no parents nor children
	bool actsAsBonusSourceOnly() const;
	//bool isLimitedOnUs(Bonus *b) const; //if bonus should be removed from list acquired from this node

	void popBonuses(const CSelector &s);
	void updateBonuses(const CSelector &s);
	virtual std::string bonusToString(const Bonus *bonus, bool description) const {return "";}; //description or bonus name
	virtual std::string nodeName() const;

	void deserializationFix();
	void exportBonus(Bonus * b);
	void exportBonuses();

	const BonusList &getBonusList() const;
	BonusList &getExportedBonusList();
	CBonusSystemNode::ENodeTypes getNodeType() const;
	void setNodeType(CBonusSystemNode::ENodeTypes type);
	const TNodesVector &getParentNodes() const;
	const TNodesVector &getChildrenNodes() const;
	const std::string &getDescription() const;
	void setDescription(const std::string &description);

	static void treeHasChanged();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & /*bonuses & */nodeType;
		h & exportedBonuses;
		h & description;
		BONUS_TREE_DESERIALIZATION_FIX
		//h & parents & children;
	}
};

namespace NBonus
{
	//set of methods that may be safely called with nullptr objs
	DLL_LINKAGE int valOf(const CBonusSystemNode *obj, Bonus::BonusType type, int subtype = -1); //subtype -> subtype of bonus, if -1 then any
	DLL_LINKAGE bool hasOfType(const CBonusSystemNode *obj, Bonus::BonusType type, int subtype = -1);//determines if hero has a bonus of given type (and optionally subtype)
	DLL_LINKAGE int getCount(const CBonusSystemNode *obj, Bonus::BonusSource from, int id);
}

/// generates HeroBonus from given data
inline Bonus makeFeatureVal(Bonus::BonusType type, ui8 duration, si16 subtype, si32 value, Bonus::BonusSource source, ui16 turnsRemain = 0, si32 additionalInfo = 0)
{
	Bonus sf;
	sf.type = type;
	sf.duration = duration;
	sf.source = source;
	sf.turnsRemain = turnsRemain;
	sf.subtype = subtype;
	sf.val = value;
	sf.additionalInfo = additionalInfo;

	return sf;
}

///generates HeroBonus from given data
inline Bonus * makeFeature(Bonus::BonusType type, ui8 duration, si16 subtype, si32 value, Bonus::BonusSource source, ui16 turnsRemain = 0, si32 additionalInfo = 0)
{
	return new Bonus(makeFeatureVal(type, duration, subtype, value, source, turnsRemain, additionalInfo));
}

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
		return [ptr2, valueToCompareAgainst](const Bonus *bonus) {  return bonus->*ptr2 == valueToCompareAgainst; };
	}
};

template<typename T>
class CSelectFieldAny //allows to ignore value of certain field, that is to accept any value
{
	T Bonus::*ptr;
public:
	CSelectFieldAny(T Bonus::*Ptr)
		: ptr(Ptr)
	{
	}
	bool operator()(const Bonus *bonus) const
	{
		return true;
	}
	CSelectFieldAny& operator()()
	{
		return *this;
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

//Stores multiple limiters. If any of them fails -> bonus is dropped.
class DLL_LINKAGE LimiterList : public ILimiter
{
	std::vector<TLimiterPtr> limiters;

public:
	int limit(const BonusLimitationContext &context) const override;
	void add(TLimiterPtr limiter);
};

class DLL_LINKAGE CCreatureTypeLimiter : public ILimiter //affect only stacks of given creature (and optionally it's upgrades)
{
public:
	const CCreature *creature;
	bool includeUpgrades;

	CCreatureTypeLimiter();
	CCreatureTypeLimiter(const CCreature &Creature, bool IncludeUpgrades = true);
	void setCreature (CreatureID id);

	int limit(const BonusLimitationContext &context) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & creature & includeUpgrades;
	}
};

class DLL_LINKAGE HasAnotherBonusLimiter : public ILimiter //applies only to nodes that have another bonus working
{
public:
	Bonus::BonusType type;
	TBonusSubtype subtype;
	bool isSubtypeRelevant; //check for subtype only if this is true

	HasAnotherBonusLimiter(Bonus::BonusType bonus = Bonus::NONE);
	HasAnotherBonusLimiter(Bonus::BonusType bonus, TBonusSubtype _subtype);

	int limit(const BonusLimitationContext &context) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & type & subtype & isSubtypeRelevant;
	}
};

class DLL_LINKAGE CreatureNativeTerrainLimiter : public ILimiter //applies only to creatures that are on their native terrain
{
public:
	int terrainType;
	CreatureNativeTerrainLimiter();
	CreatureNativeTerrainLimiter(int TerrainType);

	int limit(const BonusLimitationContext &context) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & terrainType;
	}
};

class DLL_LINKAGE CreatureFactionLimiter : public ILimiter //applies only to creatures of given faction
{
public:
	si8 faction;
	CreatureFactionLimiter();
	CreatureFactionLimiter(int TerrainType);

	int limit(const BonusLimitationContext &context) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & faction;
	}
};

class DLL_LINKAGE CreatureAlignmentLimiter : public ILimiter //applies only to creatures of given alignment
{
public:
	si8 alignment;
	CreatureAlignmentLimiter();
	CreatureAlignmentLimiter(si8 Alignment);

	int limit(const BonusLimitationContext &context) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & alignment;
	}
};

class DLL_LINKAGE StackOwnerLimiter : public ILimiter //applies only to creatures of given alignment
{
public:
	PlayerColor owner;
	StackOwnerLimiter();
	StackOwnerLimiter(PlayerColor Owner);

	int limit(const BonusLimitationContext &context) const override;

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
	int limit(const BonusLimitationContext &context) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<ILimiter&>(*this);
		h & minRank & maxRank;
	}
};

const CCreature *retrieveCreature(const CBonusSystemNode *node);

namespace Selector
{
	extern DLL_LINKAGE CSelectFieldEqual<Bonus::BonusType> type;
	extern DLL_LINKAGE CSelectFieldEqual<TBonusSubtype> subtype;
	extern DLL_LINKAGE CSelectFieldEqual<si32> info;
	extern DLL_LINKAGE CSelectFieldEqual<Bonus::BonusSource> sourceType;
	extern DLL_LINKAGE CSelectFieldEqual<Bonus::LimitEffect> effectRange;
	extern DLL_LINKAGE CWillLastTurns turns;
	extern DLL_LINKAGE CWillLastDays days;
	extern DLL_LINKAGE CSelectFieldAny<Bonus::LimitEffect> anyRange;

	CSelector DLL_LINKAGE typeSubtype(Bonus::BonusType Type, TBonusSubtype Subtype);
	CSelector DLL_LINKAGE typeSubtypeInfo(Bonus::BonusType type, TBonusSubtype subtype, si32 info);
	CSelector DLL_LINKAGE source(Bonus::BonusSource source, ui32 sourceID);
	CSelector DLL_LINKAGE sourceTypeSel(Bonus::BonusSource source);

	bool DLL_LINKAGE matchesType(const CSelector &sel, Bonus::BonusType type);
	bool DLL_LINKAGE matchesTypeSubtype(const CSelector &sel, Bonus::BonusType type, TBonusSubtype subtype);
	bool DLL_LINKAGE positiveSpellEffects(const Bonus *b);
}

extern DLL_LINKAGE const std::map<std::string, Bonus::BonusType> bonusNameMap;
extern DLL_LINKAGE const std::map<std::string, Bonus::ValueType> bonusValueMap;
extern DLL_LINKAGE const std::map<std::string, Bonus::BonusSource> bonusSourceMap;
extern DLL_LINKAGE const std::map<std::string, ui16> bonusDurationMap;
extern DLL_LINKAGE const std::map<std::string, Bonus::LimitEffect> bonusLimitEffect;
extern DLL_LINKAGE const std::map<std::string, TLimiterPtr> bonusLimiterMap;
extern DLL_LINKAGE const std::map<std::string, TPropagatorPtr> bonusPropagatorMap;


// BonusList template that requires full interface of CBonusSystemNode
template <class InputIterator>
void BonusList::insert(const int position, InputIterator first, InputIterator last)
{
	bonuses.insert(bonuses.begin() + position, first, last);

	if (belongsToTree)
		CBonusSystemNode::treeHasChanged();
}

// Extensions for BOOST_FOREACH to enable iterating of BonusList objects
/*namespace boost
{
	template<>
	struct range_mutable_iterator<BonusList>
	{
		typedef std::vector<Bonus*>::iterator type;
	};

	template<>
	struct range_const_iterator<BonusList>
	{
		typedef std::vector<Bonus*>::const_iterator type;
	};
}*/
