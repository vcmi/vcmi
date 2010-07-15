#pragma once
#include "../global.h"
#include <string>
#include <list>
#include <set>
#include <boost/function.hpp>

/*
 * HeroBonus.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


typedef ui8 TBonusType;
typedef si32 TBonusSubtype;

class CCreature;
class CSpell;
struct Bonus;
class CBonusSystemNode;
class ILimiter;

typedef std::vector<std::pair<int,std::string> > TModDescr; //modifiers values and their descriptions
typedef std::set<CBonusSystemNode*> TNodes;
typedef std::set<const CBonusSystemNode*> TCNodes;
typedef boost::function<bool(const Bonus&)> CSelector;

namespace PrimarySkill
{
	enum { ATTACK, DEFENSE, SPELL_POWER, KNOWLEDGE};
}

#define BONUS_LIST										\
	BONUS_NAME(NONE) 									\
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
	BONUS_NAME(STACKS_SPEED)  /*additional info - percent of speed bonus applied after direct bonuses; >0 - added, <0 - substracted to this part*/ \
	BONUS_NAME(FLYING_MOVEMENT) /*subtype 1 - without penalty, 2 - with penalty*/ \
	BONUS_NAME(SPELL_DURATION) \
	BONUS_NAME(AIR_SPELL_DMG_PREMY) \
	BONUS_NAME(EARTH_SPELL_DMG_PREMY) \
	BONUS_NAME(FIRE_SPELL_DMG_PREMY) \
	BONUS_NAME(WATER_SPELL_DMG_PREMY) \
	BONUS_NAME(BLOCK_SPELLS_ABOVE_LEVEL) \
	BONUS_NAME(WATER_WALKING) /*subtype 1 - without penalty, 2 - with penalty*/ \
	BONUS_NAME(NO_SHOTING_PENALTY) \
	BONUS_NAME(DISPEL_IMMUNITY) \
	BONUS_NAME(NEGATE_ALL_NATURAL_IMMUNITIES) \
	BONUS_NAME(STACK_HEALTH) \
	BONUS_NAME(BLOCK_MORALE) \
	BONUS_NAME(BLOCK_LUCK) \
	BONUS_NAME(FIRE_SPELLS) \
	BONUS_NAME(AIR_SPELLS) \
	BONUS_NAME(WATER_SPELLS) \
	BONUS_NAME(EARTH_SPELLS) \
	BONUS_NAME(GENERATE_RESOURCE) /*daily value, uses subtype (resource type)*/  \
	BONUS_NAME(CREATURE_GROWTH) /*for legion artifacts: value - week growth bonus, subtype - monster level*/  \
	BONUS_NAME(WHIRLPOOL_PROTECTION) /*hero won't lose army when teleporting through whirlpool*/  \
	BONUS_NAME(SPELL) /*hero knows spell, val - skill level (0 - 3), subtype - spell id*/  \
	BONUS_NAME(SPELLS_OF_LEVEL) /*hero knows all spells of given level, val - skill level; subtype - level*/  \
	BONUS_NAME(ENEMY_CANT_ESCAPE) /*for shackles of war*/ \
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
	BONUS_NAME(HATE) /*eg. angels hate devils, subtype - ID of hated creature*/ \
	BONUS_NAME(KING1)									\
	BONUS_NAME(KING2)									\
	BONUS_NAME(KING3)									\
	BONUS_NAME(MAGIC_RESISTANCE) /*in % (value)*/		\
	BONUS_NAME(CHANGES_SPELL_COST_FOR_ALLY) /*in mana points (value) , eg. mage*/ \
	BONUS_NAME(CHANGES_SPELL_COST_FOR_ENEMY) /*in mana points (value) , eg. pegasus */ \
	BONUS_NAME(SPELL_AFTER_ATTACK) /* subtype - spell id, value - spell level, (additional info)%1000 - chance in %; eg. dendroids, (additional info)/1000 -> [0 - all attacks, 1 - shot only, 2 - melee only*/ \
	BONUS_NAME(SPELL_RESISTANCE_AURA) /*eg. unicorns, value - resistance bonus in % for adjacent creatures*/ \
	BONUS_NAME(LEVEL_SPELL_IMMUNITY) /*creature is immune to all spell with level below or equal to value of this bonus*/ \
	BONUS_NAME(TWO_HEX_ATTACK_BREATH) /*eg. dragons*/	\
	BONUS_NAME(SPELL_DAMAGE_REDUCTION) /*eg. golems; value - reduction in %, subtype - spell school; -1 - all, 0 - air, 1 - fire, 2 - water, 3 - earth*/ \
	BONUS_NAME(NO_WALL_PENALTY)							\
	BONUS_NAME(NON_LIVING) /*eg. gargoyle*/				\
	BONUS_NAME(RANDOM_GENIE_SPELLCASTER) /*eg. master genie*/ \
	BONUS_NAME(BLOCKS_RETALIATION) /*eg. naga*/			\
	BONUS_NAME(SPELL_IMMUNITY) /*subid - spell id*/		\
	BONUS_NAME(MANA_CHANNELING) /*value in %, eg. familiar*/ \
	BONUS_NAME(SPELL_LIKE_ATTACK) /*value - spell id; range is taken from spell, but damage from creature; eg. magog*/ \
	BONUS_NAME(THREE_HEADED_ATTACK) /*eg. cerberus*/	\
	BONUS_NAME(DAEMON_SUMMONING) /*pit lord*/			\
	BONUS_NAME(FIRE_IMMUNITY)							\
	BONUS_NAME(FIRE_SHIELD)								\
	BONUS_NAME(UNDEAD)									\
	BONUS_NAME(HP_REGENERATION) /*creature regenerates val HP every new round*/					\
	BONUS_NAME(FULL_HP_REGENERATION) /*first creature regenerates all HP every new round; subtype 0 - animation 4 (trolllike), 1 - animation 47 (wightlike)*/		\
	BONUS_NAME(MANA_DRAIN) /*value - spell points per turn*/ \
	BONUS_NAME(LIFE_DRAIN)								\
	BONUS_NAME(DOUBLE_DAMAGE_CHANCE) /*value in %, eg. dread knight*/ \
	BONUS_NAME(RETURN_AFTER_STRIKE)						\
	BONUS_NAME(SELF_MORALE) /*eg. minotaur*/			\
	BONUS_NAME(SPELLCASTER) /*subtype - spell id, value - level of school, additional info - spell power*/ \
	BONUS_NAME(CATAPULT)								\
	BONUS_NAME(ENEMY_DEFENCE_REDUCTION) /*in % (value) eg. behemots*/ \
	BONUS_NAME(GENERAL_DAMAGE_REDUCTION) /* shield / air shield effect */ \
	BONUS_NAME(GENERAL_ATTACK_REDUCTION) /*eg. while stoned or blinded - in %, subtype: -1 - any damage, 0 - melee damage, 1 - ranged damage*/ \
	BONUS_NAME(ATTACKS_ALL_ADJACENT) /*eg. hydra*/		\
	BONUS_NAME(MORE_DAMAGE_FROM_SPELL) /*value - damage increase in %, subtype - spell id*/ \
	BONUS_NAME(CASTS_SPELL_WHEN_KILLED) /*similar to spell after attack*/ \
	BONUS_NAME(FEAR)									\
	BONUS_NAME(FEARLESS)								\
	BONUS_NAME(NO_DISTANCE_PENALTY)						\
	BONUS_NAME(NO_OBSTACLES_PENALTY)					\
	BONUS_NAME(SELF_LUCK) /*halfling*/					\
	BONUS_NAME(ENCHANTER)								\
	BONUS_NAME(HEALER)									\
	BONUS_NAME(SIEGE_WEAPON)							\
	BONUS_NAME(HYPNOTIZED)								\
	BONUS_NAME(ADDITIONAL_RETALIATION) /*value - number of additional retaliations*/ \
	BONUS_NAME(MAGIC_MIRROR) /* value - chance of redirecting in %*/ \
	BONUS_NAME(ALWAYS_MINIMUM_DAMAGE) /*unit does its minimum damage from range; subtype: -1 - any attack, 0 - melee, 1 - ranged, value: additional damage, additional info - multiplicative anti-bonus for dmg in % [eg 20 means that creature will inflict 80% of normal dmg]*/ \
	BONUS_NAME(ALWAYS_MAXIMUM_DAMAGE) /*eg. bless effect, subtype: -1 - any attack, 0 - melee, 1 - ranged, value: additional damage, additional info - multiplicative bonus for dmg in %*/ \
	BONUS_NAME(ATTACKS_NEAREST_CREATURE) /*while in berserk*/ \
	BONUS_NAME(IN_FRENZY) /*value - level*/				\
	BONUS_NAME(SLAYER) /*value - level*/				\
	BONUS_NAME(FORGETFULL) /*forgetfulness spell effect, value - level*/ \
	BONUS_NAME(NOT_ACTIVE)								\
	BONUS_NAME(NO_LUCK) /*eg. when fighting on cursed ground*/	\
	BONUS_NAME(NO_MORALE) /*eg. when fighting on cursed ground*/ \
	BONUS_NAME(DARKNESS) /*val = radius */ \
	BONUS_NAME(SPECIAL_SECONDARY_SKILL) /*val = id, additionalInfo = value per level in percent*/ \
	BONUS_NAME(SPECIAL_SPELL_LEV) /*val = id, additionalInfo = value per level in percent*/\
	BONUS_NAME(SPECIFIC_SPELL_DAMAGE) /*val = id of spell, additionalInfo = value*/\
	BONUS_NAME(SPECIAL_BLESS_DAMAGE) /*val = spell (bless), additionalInfo = value per level in percent*/\
	BONUS_NAME(MAXED_SPELL) /*val = id*/\
	BONUS_NAME(SPECIAL_PECULIAR_ENCHANT) /*blesses and curses with id = val dependent on unit's level, subtype = 0 or 1 for Coronius*/\
	BONUS_NAME(SPECIAL_UPGRADE) /*val = base, additionalInfo = target */\
	BONUS_NAME(DRAGON_NATURE) /*TODO: implement it!*/\
	BONUS_NAME(CREATURE_DAMAGE)/*subtype 0 = both, 1 = min, 2 = max*/

struct DLL_EXPORT Bonus
{
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
		UNTIL_ATTACK = 128 /*removed after attack and counterattacks are performed*/
	};
	enum BonusSource
	{
		ARTIFACT, 
		OBJECT, 
		CASTED_SPELL,
		CREATURE_ABILITY,
		TERRAIN_NATIVE,
		TERRAIN_OVERLAY,
		SPELL_EFFECT,
		TOWN_STRUCTURE,
		HERO_BASE_SKILL,
		SECONDARY_SKILL,
		HERO_SPECIAL,
		ARMY
	};

	enum LimitEffect
	{
		NO_LIMIT = 0, 
		ONLY_DISTANCE_FIGHT=1, ONLY_MELEE_FIGHT, //used to mark bonuses for attack/defense primary skills from spells like Precision (distance only)
		ONLY_ALLIED_ARMY, ONLY_ENEMY_ARMY,
		PLAYR_HEROES
	};

	enum ValueType
	{
		ADDITIVE_VALUE,
		BASE_NUMBER,
		PERCENT_TO_ALL,
		PERCENT_TO_BASE
	};

	ui8 duration; //uses BonusDuration values
	si16 turnsRemain; //used if duration is N_TURNS or N_DAYS

	TBonusType type; //uses BonusType values - says to what is this bonus - 1 byte
	TBonusSubtype subtype; //-1 if not applicable - 4 bytes

	ui8 source;//source type" uses BonusSource values - what gave that bonus
	si32 val;
	ui32 id; //source id: id of object/artifact/spell
	ui8 valType; 

	si32 additionalInfo;
	ui8 effectRange; //if not NO_LIMIT, bonus will be ommitted by default

	ILimiter *limiter;

	std::string description; 

	Bonus(ui8 Dur, ui8 Type, ui8 Src, si32 Val, ui32 ID, std::string Desc, si32 Subtype=-1);
	Bonus(ui8 Dur, ui8 Type, ui8 Src, si32 Val, ui32 ID, si32 Subtype=-1, ui8 ValType = ADDITIVE_VALUE);
	Bonus();

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
		h & duration & type & subtype & source & val & id & description & additionalInfo & turnsRemain & valType & effectRange & limiter;
	}

	static bool OneDay(const Bonus &hb)
	{
		return hb.duration & Bonus::ONE_DAY;
	}
	static bool OneWeek(const Bonus &hb)
	{
		return hb.duration & Bonus::ONE_WEEK;
	}
	static bool OneBattle(const Bonus &hb)
	{
		return hb.duration & Bonus::ONE_BATTLE;
	}
	static bool UntilAttack(const Bonus &hb)
	{
		return hb.duration & Bonus::UNTIL_ATTACK;
	}
	static bool UntilBeingAttacked(const Bonus &hb)
	{
		return hb.duration & Bonus::UNITL_BEING_ATTACKED;
	}
	static bool IsFrom(const Bonus &hb, ui8 source, ui32 id) //if id==0xffffff then id doesn't matter
	{
		return hb.source==source && (id==0xffffff  ||  hb.id==id);
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
};

DLL_EXPORT std::ostream & operator<<(std::ostream &out, const Bonus &bonus);

class BonusList : public std::list<Bonus>
{
public:
	int DLL_EXPORT totalValue() const; //subtype -> subtype of bonus, if -1 then any
	void DLL_EXPORT getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *source = NULL) const;
	void DLL_EXPORT getBonuses(BonusList &out, const CSelector &selector, const CSelector &limit, const CBonusSystemNode *source = NULL) const;
	void DLL_EXPORT getModifiersWDescr(TModDescr &out) const;

	//special find functions
	DLL_EXPORT Bonus * getFirst(const CSelector &select);
	DLL_EXPORT const Bonus * getFirst(const CSelector &select) const;

	void limit(const CBonusSystemNode &node); //erases bonuses using limitor

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<std::list<Bonus>&>(*this);
	}
};

DLL_EXPORT std::ostream & operator<<(std::ostream &out, const BonusList &bonusList);

class DLL_EXPORT ILimiter
{
public:
	virtual ~ILimiter();

	virtual bool limit(const Bonus &b, const CBonusSystemNode &node) const; //return true to drop the bonus

	template <typename Handler> void serialize(Handler &h, const int version)
	{}
};

class DLL_EXPORT CBonusSystemNode
{
public:
	BonusList bonuses;
	ui8 nodeType;

	CBonusSystemNode();
	virtual ~CBonusSystemNode();

	//new bonusing node interface
	// * selector is predicate that tests if HeroBonus matches our criteria
	// * root is node on which call was made (NULL will be replaced with this)
	virtual void getParents(TCNodes &out, const CBonusSystemNode *root = NULL) const;  //retrieves list of parent nodes (nodes to inherit bonuses from), source is the prinary asker
	virtual void getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *root = NULL) const;

	void getBonuses(BonusList &out, const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = NULL) const;
	BonusList getBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = NULL) const;
	BonusList getBonuses(const CSelector &selector, const CBonusSystemNode *root = NULL) const;
	int getBonusesCount(const CSelector &selector, const CBonusSystemNode *root = NULL) const;
	int valOfBonuses(const CSelector &selector, const CBonusSystemNode *root = NULL) const;
	bool hasBonus(const CSelector &selector, const CBonusSystemNode *root = NULL) const;
	void getModifiersWDescr(TModDescr &out, const CSelector &selector, const CBonusSystemNode *root = NULL) const;  //out: pairs<modifier value, modifier description>

	//////////////////////////////////////////////////////////////////////////
	//legacy interface 
	int valOfBonuses(Bonus::BonusType type, const CSelector &selector) const;
	int valOfBonuses(Bonus::BonusType type, int subtype = -1) const; //subtype -> subtype of bonus, if -1 then any
	bool hasBonusOfType(Bonus::BonusType type, int subtype = -1) const;//determines if hero has a bonus of given type (and optionally subtype)
	bool hasBonusFrom(ui8 source, ui32 sourceID) const;
	void getModifiersWDescr( TModDescr &out, Bonus::BonusType type, int subtype = -1 ) const;  //out: pairs<modifier value, modifier description>
	int getBonusesCount(int from, int id) const;

	int MoraleVal() const; //range [-3, +3]
	int LuckVal() const; //range [-3, +3]
	si32 Attack() const; //get attack of stack with all modificators
	si32 Defense(bool withFrenzy = true) const; //get defense of stack with all modificators
	ui16 MaxHealth() const; //get max HP of stack with all modifiers


	//non-const interface
	void getParents(TNodes &out, const CBonusSystemNode *root = NULL);  //retrieves list of parent nodes (nodes to inherit bonuses from), source is the prinary asker
	Bonus *getBonus(const CSelector &selector);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & bonuses & nodeType;
	}

	enum ENodeTypes
	{
		UNKNOWN, STACK, SPECIALITY
	};
};

namespace NBonus
{
	//set of methods that may be safely called with NULL objs
	DLL_EXPORT int valOf(const CBonusSystemNode *obj, Bonus::BonusType type, int subtype = -1); //subtype -> subtype of bonus, if -1 then any
	DLL_EXPORT bool hasOfType(const CBonusSystemNode *obj, Bonus::BonusType type, int subtype = -1);//determines if hero has a bonus of given type (and optionally subtype)
	//DLL_EXPORT const HeroBonus * get(const CBonusSystemNode *obj, int from, int id );
	DLL_EXPORT void getModifiersWDescr(const CBonusSystemNode *obj, TModDescr &out, Bonus::BonusType type, int subtype = -1 );  //out: pairs<modifier value, modifier description>
	DLL_EXPORT int getCount(const CBonusSystemNode *obj, int from, int id);
};

//generates HeroBonus from given data
inline Bonus makeFeature(Bonus::BonusType type, ui8 duration, si16 subtype, si32 value, Bonus::BonusSource source, ui16 turnsRemain = 0, si32 additionalInfo = 0)
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

class DLL_EXPORT CSelectorsConjunction
{
	const CSelector first, second;
public:
	CSelectorsConjunction(const CSelector &First, const CSelector &Second)
		:first(First), second(Second)
	{
	}
	bool operator()(const Bonus &bonus) const
	{
		return first(bonus) && second(bonus);
	}
};
CSelector DLL_EXPORT operator&&(const CSelector &first, const CSelector &second);


template<typename T>
class CSelectFieldEqual
{
	T Bonus::*ptr;
	T val;
public:
	CSelectFieldEqual(T Bonus::*Ptr, const T &Val)
		: ptr(Ptr), val(Val)
	{
	}
	bool operator()(const Bonus &bonus) const
	{
		return bonus.*ptr == val;
	}
	CSelectFieldEqual& operator()(const T &setVal)
	{
		val = setVal;
		return *this;
	}
};

class CWillLastTurns
{
public:
	int turnsRequested;

	bool operator()(const Bonus &bonus) const
	{
		return turnsRequested <= 0					//every present effect will last zero (or "less") turns
			|| !(bonus.duration & Bonus::N_TURNS)	//so do every not expriing after N-turns effect
			|| bonus.turnsRemain > turnsRequested;	
	}
	CWillLastTurns& operator()(const int &setVal)
	{
		turnsRequested = setVal;
		return *this;
	}
};

class CCreatureTypeLimiter : public ILimiter //affect only stacks of given creature (and optionally it's upgrades)
{
public:
	const CCreature *creature;
	ui8 includeUpgrades;

	CCreatureTypeLimiter();
	CCreatureTypeLimiter(const CCreature &Creature, ui8 IncludeUpgrades = true);

	bool limit(const Bonus &b, const CBonusSystemNode &node) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & creature & includeUpgrades;
	}
};
namespace Selector
{
	extern DLL_EXPORT CSelectFieldEqual<TBonusType> type;
	extern DLL_EXPORT CSelectFieldEqual<TBonusSubtype> subtype;
	extern DLL_EXPORT CSelectFieldEqual<si32> info;
	extern DLL_EXPORT CSelectFieldEqual<ui8> sourceType;
	extern DLL_EXPORT CSelectFieldEqual<ui8> effectRange;
	extern DLL_EXPORT CWillLastTurns turns;

	CSelector DLL_EXPORT typeSybtype(TBonusType Type, TBonusSubtype Subtype);
	CSelector DLL_EXPORT typeSybtypeInfo(TBonusType type, TBonusSubtype subtype, si32 info);
	CSelector DLL_EXPORT source(ui8 source, ui32 sourceID);

	bool DLL_EXPORT matchesType(const CSelector &sel, TBonusType type);
	bool DLL_EXPORT matchesTypeSubtype(const CSelector &sel, TBonusType type, TBonusSubtype subtype);
}

extern DLL_EXPORT const std::map<std::string, int> bonusNameMap;