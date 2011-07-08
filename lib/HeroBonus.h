#pragma once
#include "../global.h"
#include <string>
#include <set>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

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

typedef std::vector<std::pair<int,std::string> > TModDescr; //modifiers values and their descriptions
typedef std::set<CBonusSystemNode*> TNodes;
typedef std::set<const CBonusSystemNode*> TCNodes;
typedef std::vector<CBonusSystemNode *> TNodesVector;
typedef boost::function<bool(const Bonus*)> CSelector;

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
	BONUS_NAME(WATER_WALKING) /*subtype 1 - without penalty, 2 - with penalty*/ \
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
	BONUS_NAME(HATE) /*eg. angels hate devils, subtype - ID of hated creature, val - damage bonus percent */ \
	BONUS_NAME(KING1)									\
	BONUS_NAME(KING2)									\
	BONUS_NAME(KING3)									\
	BONUS_NAME(MAGIC_RESISTANCE) /*in % (value)*/		\
	BONUS_NAME(CHANGES_SPELL_COST_FOR_ALLY) /*in mana points (value) , eg. mage*/ \
	BONUS_NAME(CHANGES_SPELL_COST_FOR_ENEMY) /*in mana points (value) , eg. pegasus */ \
	BONUS_NAME(SPELL_AFTER_ATTACK) /* subtype - spell id, value - chance %, additional info % 1000 - level, (additional info)/1000 -> [0 - all attacks, 1 - shot only, 2 - melee only*/ \
	BONUS_NAME(SPELL_RESISTANCE_AURA) /*eg. unicorns, value - resistance bonus in % for adjacent creatures*/ \
	BONUS_NAME(LEVEL_SPELL_IMMUNITY) /*creature is immune to all spell with level below or equal to value of this bonus*/ \
	BONUS_NAME(TWO_HEX_ATTACK_BREATH) /*eg. dragons*/	\
	BONUS_NAME(SPELL_DAMAGE_REDUCTION) /*eg. golems; value - reduction in %, subtype - spell school; -1 - all, 0 - air, 1 - fire, 2 - water, 3 - earth*/ \
	BONUS_NAME(NO_WALL_PENALTY)							\
	BONUS_NAME(NON_LIVING) /*eg. gargoyle*/				\
	BONUS_NAME(RANDOM_SPELLCASTER) /*eg. master genie, For Genie spells, subtype - spell id */ \
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
	BONUS_NAME(FIRE_SHIELD)								\
	BONUS_NAME(UNDEAD)									\
	BONUS_NAME(HP_REGENERATION) /*creature regenerates val HP every new round*/					\
	BONUS_NAME(FULL_HP_REGENERATION) /*first creature regenerates all HP every new round; subtype 0 - animation 4 (trolllike), 1 - animation 47 (wightlike)*/		\
	BONUS_NAME(MANA_DRAIN) /*value - spell points per turn*/ \
	BONUS_NAME(LIFE_DRAIN)								\
	BONUS_NAME(DOUBLE_DAMAGE_CHANCE) /*value in %, eg. dread knight*/ \
	BONUS_NAME(RETURN_AFTER_STRIKE)						\
	BONUS_NAME(SELF_MORALE) /*eg. minotaur*/			\
	BONUS_NAME(SPELLCASTER) /*subtype - spell id, value - level of school. use SPECIFIC_SPELL_POWER, CREATURE_SPELL_POWER or CREATURE_ENCHANT_POWER for calculating the power*/ \
	BONUS_NAME(CATAPULT)								\
	BONUS_NAME(ENEMY_DEFENCE_REDUCTION) /*in % (value) eg. behemots*/ \
	BONUS_NAME(GENERAL_DAMAGE_REDUCTION) /* shield / air shield effect */ \
	BONUS_NAME(GENERAL_ATTACK_REDUCTION) /*eg. while stoned or blinded - in %, subtype: -1 - any damage, 0 - melee damage, 1 - ranged damage*/ \
	BONUS_NAME(ATTACKS_ALL_ADJACENT) /*eg. hydra*/		\
	BONUS_NAME(MORE_DAMAGE_FROM_SPELL) /*value - damage increase in %, subtype - spell id*/ \
	BONUS_NAME(FEAR)									\
	BONUS_NAME(FEARLESS)								\
	BONUS_NAME(NO_DISTANCE_PENALTY)						\
	BONUS_NAME(NO_OBSTACLES_PENALTY)					\
	BONUS_NAME(SELF_LUCK) /*halfling*/					\
	BONUS_NAME(ENCHANTER)/* for Enchanter spells, subtype - spell id */ \
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
	BONUS_NAME(SPECIFIC_SPELL_POWER) /* value used for Thunderbolt and Resurrection casted by units, subtype - spell id */\
	BONUS_NAME(CREATURE_SPELL_POWER) /* value per unit, divided by 100 (so faerie Dragons have 800)*/ \
	BONUS_NAME(CREATURE_ENCHANT_POWER) /* total duration of spells casted by creature */ \
	BONUS_NAME(REBIRTH) /* val - percent of life restored, subtype = 0 - regular, 1 - at least one unit (sacred Phoenix) */

/// Struct for handling bonuses of several types. Can be transfered to any hero
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
		UNTIL_ATTACK = 128, /*removed after attack and counterattacks are performed*/
		STACK_GETS_TURN = 256 /*removed when stack gets its turn - used for defensive stance*/
	};
	enum BonusSource
	{
		ARTIFACT,
		ARTIFACT_INSTANCE,
		OBJECT,
		CREATURE_ABILITY,
		TERRAIN_NATIVE,
		TERRAIN_OVERLAY,
		SPELL_EFFECT,
		TOWN_STRUCTURE,
		HERO_BASE_SKILL,
		SECONDARY_SKILL,
		HERO_SPECIAL,
		ARMY,
		CAMPAIGN_BONUS,
		SPECIAL_WEEK,
		STACK_EXPERIENCE,
		OTHER /*used for defensive stance and default value of spell level limit*/
	};

	enum LimitEffect
	{
		NO_LIMIT = 0, 
		ONLY_DISTANCE_FIGHT=1, ONLY_MELEE_FIGHT, //used to mark bonuses for attack/defense primary skills from spells like Precision (distance only)
		ONLY_ENEMY_ARMY
	};

	enum ValueType
	{
		ADDITIVE_VALUE,
		BASE_NUMBER,
		PERCENT_TO_ALL,
		PERCENT_TO_BASE,
		INDEPENDENT_MAX, //used for SPELL bonus
		INDEPENDENT_MIN //used for SECONDARY_SKILL_PREMY bonus
	};

	ui16 duration; //uses BonusDuration values
	si16 turnsRemain; //used if duration is N_TURNS or N_DAYS

	TBonusType type; //uses BonusType values - says to what is this bonus - 1 byte
	TBonusSubtype subtype; //-1 if not applicable - 4 bytes

	ui8 source;//source type" uses BonusSource values - what gave that bonus
	si32 val;
	ui32 sid; //source id: id of object/artifact/spell
	ui8 valType; //by ValueType enum

	si32 additionalInfo;
	ui8 effectRange; //if not NO_LIMIT, bonus will be ommitted by default

	boost::shared_ptr<ILimiter> limiter;
	boost::shared_ptr<IPropagator> propagator;

	std::string description; 

	Bonus(ui16 Dur, ui8 Type, ui8 Src, si32 Val, ui32 ID, std::string Desc, si32 Subtype=-1);
	Bonus(ui16 Dur, ui8 Type, ui8 Src, si32 Val, ui32 ID, si32 Subtype=-1, ui8 ValType = ADDITIVE_VALUE);
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

	Bonus *addLimiter(ILimiter *Limiter); //returns this for convenient chain-calls
	Bonus *addPropagator(IPropagator *Propagator); //returns this for convenient chain-calls
	Bonus *addLimiter(boost::shared_ptr<ILimiter> Limiter); //returns this for convenient chain-calls
	Bonus *addPropagator(boost::shared_ptr<IPropagator> Propagator); //returns this for convenient chain-calls
};

DLL_EXPORT std::ostream & operator<<(std::ostream &out, const Bonus &bonus);

class BonusList : public std::vector<Bonus*>
{
public:
	int DLL_EXPORT totalValue() const; //subtype -> subtype of bonus, if -1 then any
	void DLL_EXPORT getBonuses(boost::shared_ptr<BonusList> out, const CSelector &selector, const CSelector &limit, const bool caching = false) const;
	void DLL_EXPORT getModifiersWDescr(TModDescr &out) const;

	void DLL_EXPORT getBonuses(boost::shared_ptr<BonusList> out, const CSelector &selector) const;

	//special find functions
	DLL_EXPORT Bonus * getFirst(const CSelector &select);
	DLL_EXPORT const Bonus * getFirst(const CSelector &select) const;

	void limit(const CBonusSystemNode &node); //erases bonuses using limitor
	void DLL_EXPORT eliminateDuplicates();
	
	// remove_if implementation for STL vector types
	template <class Predicate>
	void remove_if(Predicate pred)
	{
		BonusList newList;
		for (int i = 0; i < this->size(); i++)
		{
			Bonus *b = (*this)[i];
			if (!pred(b))
				newList.push_back(b);
		}
		this->clear();
		this->resize(newList.size());
		std::copy(newList.begin(), newList.end(), this->begin());
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<std::vector<Bonus*>&>(*this);
	}
};

DLL_EXPORT std::ostream & operator<<(std::ostream &out, const BonusList &bonusList);

class DLL_EXPORT IPropagator
{
public:
	virtual ~IPropagator();
	virtual bool shouldBeAttached(CBonusSystemNode *dest);
	//virtual CBonusSystemNode *getDestNode(CBonusSystemNode *source, CBonusSystemNode *redParent, CBonusSystemNode *redChild); //called when red relation between parent-childrem is established / removed

	template <typename Handler> void serialize(Handler &h, const int version)
	{}
};

class DLL_EXPORT CPropagatorNodeType : public IPropagator
{
	ui8 nodeType;
public:
	CPropagatorNodeType();
	CPropagatorNodeType(ui8 NodeType);
	bool shouldBeAttached(CBonusSystemNode *dest);
	//CBonusSystemNode *getDestNode(CBonusSystemNode *source, CBonusSystemNode *redParent, CBonusSystemNode *redChild) OVERRIDE; 

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & nodeType;
	}
};
	
class DLL_EXPORT ILimiter
{
public:
	virtual ~ILimiter();

	virtual bool limit(const Bonus *b, const CBonusSystemNode &node) const; //return true to drop the bonus

	template <typename Handler> void serialize(Handler &h, const int version)
	{}
};

class DLL_EXPORT IBonusBearer
{
public:
	//new bonusing node interface
	// * selector is predicate that tests if HeroBonus matches our criteria
	// * root is node on which call was made (NULL will be replaced with this)
	//interface
	virtual const boost::shared_ptr<BonusList> getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = NULL) const = 0; 
	virtual void setCachingStr(const std::string &request) const;
	void getModifiersWDescr(TModDescr &out, const CSelector &selector) const;  //out: pairs<modifier value, modifier description>
	int getBonusesCount(const CSelector &selector) const;
	int valOfBonuses(const CSelector &selector) const;
	bool hasBonus(const CSelector &selector) const;
	const boost::shared_ptr<BonusList> getBonuses(const CSelector &selector, const CSelector &limit) const;
	const boost::shared_ptr<BonusList> getBonuses(const CSelector &selector) const;

	//legacy interface 
	int valOfBonuses(Bonus::BonusType type, const CSelector &selector) const;
	int valOfBonuses(Bonus::BonusType type, int subtype = -1) const; //subtype -> subtype of bonus, if -1 then anyt;
	bool hasBonusOfType(Bonus::BonusType type, int subtype = -1) const;//determines if hero has a bonus of given type (and optionally subtype)
	bool hasBonusFrom(ui8 source, ui32 sourceID) const;
	void getModifiersWDescr( TModDescr &out, Bonus::BonusType type, int subtype = -1 ) const;  //out: pairs<modifier value, modifier description>
	int getBonusesCount(int from, int id) const;

	//various hlp functions for non-trivial values
	ui32 getMinDamage() const; //used for stacks and creatures only
	ui32 getMaxDamage() const;
	int MoraleVal() const; //range [-3, +3]
	int LuckVal() const; //range [-3, +3]
	si32 Attack() const; //get attack of stack with all modificators
	si32 Defense(bool withFrenzy = true) const; //get defense of stack with all modificators
	ui16 MaxHealth() const; //get max HP of stack with all modifiers
	bool isLiving() const; //non-undead, non-non living or alive
	virtual si32 magicResistance() const;

	si32 manaLimit() const; //maximum mana value for this hero (basically 10*knowledge)
	int getPrimSkillLevel(int id) const; //0-attack, 1-defence, 2-spell power, 3-knowledge
	const boost::shared_ptr<BonusList> getSpellBonuses() const;
};

class DLL_EXPORT CBonusSystemNode : public IBonusBearer
{
	static const bool cachingEnabled; 
	mutable BonusList cachedBonuses;
	mutable int cachedLast;	
	static int treeChanged;

	// Setting a value to cachingStr before getting any bonuses caches the result for later requests. 
	// This string needs to be unique, that's why it has to be setted in the following manner:
	// [property key]_[value] => only for selector
	mutable std::string cachingStr;
	mutable std::map<std::string, boost::shared_ptr<BonusList> > cachedRequests;

	void getAllBonusesRec(boost::shared_ptr<BonusList> out, const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = NULL, const bool caching = false) const;

public:
	BonusList bonuses; //wielded bonuses (local or up-propagated here)
	BonusList exportedBonuses; //bonuses coming from this node (wielded or propagated away)

	TNodesVector parents; //parents -> we inherit bonuses from them, we may attach our bonuses to them
	TNodesVector children;

	ui8 nodeType;
	std::string description;

	explicit CBonusSystemNode();
	virtual ~CBonusSystemNode();
	
	const boost::shared_ptr<BonusList> getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = NULL) const;
	void setCachingStr(const std::string &request) const;
	void getParents(TCNodes &out) const;  //retrieves list of parent nodes (nodes to inherit bonuses from),
	const Bonus *getBonus(const CSelector &selector) const;

	//non-const interface
	void getParents(TNodes &out);  //retrieves list of parent nodes (nodes to inherit bonuses from)
	void getRedParents(TNodes &out);  //retrieves list of red parent nodes (nodes bonuses propagate from)
	void getRedAncestors(TNodes &out);
	void getRedChildren(TNodes &out); 
	void getRedDescendants(TNodes &out); 
	Bonus *getBonus(const CSelector &selector);

	void attachTo(CBonusSystemNode *parent);
	void detachFrom(CBonusSystemNode *parent);
	void detachFromAll();
	void addNewBonus(Bonus *b); //b will be deleted with destruction of node

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
	bool isLimitedOnUs(Bonus *b) const; //if bonus should be removed from list acquired from this node

	void popBonuses(const CSelector &s);
	virtual std::string bonusToString(Bonus *bonus, bool description) const {return "";}; //description or bonus name
	virtual std::string nodeName() const;

	void deserializationFix();
	void exportBonus(Bonus * b);
	void exportBonuses();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & /*bonuses & */nodeType;
		h & exportedBonuses;
		h & description;
		BONUS_TREE_DESERIALIZATION_FIX
		//h & parents & children;
	}
	enum ENodeTypes
	{
		UNKNOWN, STACK_INSTANCE, STACK_BATTLE, SPECIALITY, ARTIFACT, CREATURE, ARTIFACT_INSTANCE, HERO, PLAYER, TEAM
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

//generates HeroBonus from given data
inline Bonus * makeFeature(Bonus::BonusType type, ui8 duration, si16 subtype, si32 value, Bonus::BonusSource source, ui16 turnsRemain = 0, si32 additionalInfo = 0)
{
	return new Bonus(makeFeatureVal(type, duration, subtype, value, source, turnsRemain, additionalInfo));
}


class DLL_EXPORT CSelectorsConjunction
{
	const CSelector first, second;
public:
	CSelectorsConjunction(const CSelector &First, const CSelector &Second)
		:first(First), second(Second)
	{
	}
	bool operator()(const Bonus *bonus) const
	{
		return first(bonus) && second(bonus);
	}
};
CSelector DLL_EXPORT operator&&(const CSelector &first, const CSelector &second);

class DLL_EXPORT CSelectorsAlternative
{
	const CSelector first, second;
public:
	CSelectorsAlternative(const CSelector &First, const CSelector &Second)
		:first(First), second(Second)
	{
	}
	bool operator()(const Bonus *bonus) const
	{
		return first(bonus) || second(bonus);
	}
};
CSelector DLL_EXPORT operator||(const CSelector &first, const CSelector &second);

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
	bool operator()(const Bonus *bonus) const
	{
		return bonus->*ptr == val;
	}
	CSelectFieldEqual& operator()(const T &setVal)
	{
		val = setVal;
		return *this;
	}
};

class DLL_EXPORT CWillLastTurns
{
public:
	int turnsRequested;

	bool operator()(const Bonus *bonus) const
	{
		return turnsRequested <= 0					//every present effect will last zero (or "less") turns
			|| !(bonus->duration & Bonus::N_TURNS)	//so do every not expriing after N-turns effect
			|| bonus->turnsRemain > turnsRequested;	
	}
	CWillLastTurns& operator()(const int &setVal)
	{
		turnsRequested = setVal;
		return *this;
	}
};

class DLL_EXPORT CCreatureTypeLimiter : public ILimiter //affect only stacks of given creature (and optionally it's upgrades)
{
public:
	const CCreature *creature;
	ui8 includeUpgrades;

	CCreatureTypeLimiter();
	CCreatureTypeLimiter(const CCreature &Creature, ui8 IncludeUpgrades = true);

	bool limit(const Bonus *b, const CBonusSystemNode &node) const OVERRIDE;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & creature & includeUpgrades;
	}
};

class DLL_EXPORT HasAnotherBonusLimiter : public ILimiter //applies only to nodes that have another bonus working
{
public:
	TBonusType type;
	TBonusSubtype subtype;
	ui8 isSubtypeRelevant; //check for subtype only if this is true

	HasAnotherBonusLimiter(TBonusType bonus = Bonus::NONE);
	HasAnotherBonusLimiter(TBonusType bonus, TBonusSubtype _subtype);

	bool limit(const Bonus *b, const CBonusSystemNode &node) const OVERRIDE;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type & subtype & isSubtypeRelevant;
	}
};

class DLL_EXPORT CreatureNativeTerrainLimiter : public ILimiter //applies only to creatures that are on their native terrain 
{
public:
	si8 terrainType;
	CreatureNativeTerrainLimiter();
	CreatureNativeTerrainLimiter(int TerrainType);

	bool limit(const Bonus *b, const CBonusSystemNode &node) const OVERRIDE;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & terrainType;
	}
};

class DLL_EXPORT CreatureFactionLimiter : public ILimiter //applies only to creatures of given faction
{
public:
	si8 faction;
	CreatureFactionLimiter();
	CreatureFactionLimiter(int TerrainType);

	bool limit(const Bonus *b, const CBonusSystemNode &node) const OVERRIDE;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & faction;
	}
};

class DLL_EXPORT CreatureAlignmentLimiter : public ILimiter //applies only to creatures of given alignment
{
public:
	si8 alignment;
	CreatureAlignmentLimiter();
	CreatureAlignmentLimiter(si8 Alignment);

	bool limit(const Bonus *b, const CBonusSystemNode &node) const OVERRIDE;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & alignment;
	}
};

class DLL_EXPORT StackOwnerLimiter : public ILimiter //applies only to creatures of given alignment
{
public:
	ui8 owner;
	StackOwnerLimiter();
	StackOwnerLimiter(ui8 Owner);

	bool limit(const Bonus *b, const CBonusSystemNode &node) const OVERRIDE;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & owner;
	}
};

class DLL_EXPORT RankRangeLimiter : public ILimiter //applies to creatures with min <= Rank <= max
{
public:
	ui8 minRank, maxRank;

	RankRangeLimiter();
	RankRangeLimiter(ui8 Min, ui8 Max = 255);
	bool limit(const Bonus *b, const CBonusSystemNode &node) const OVERRIDE;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & minRank & maxRank;
	}
};

const CCreature *retrieveCreature(const CBonusSystemNode *node);

namespace Selector
{
	extern DLL_EXPORT CSelectFieldEqual<TBonusType> type;
	extern DLL_EXPORT CSelectFieldEqual<TBonusSubtype> subtype;
	extern DLL_EXPORT CSelectFieldEqual<si32> info;
	extern DLL_EXPORT CSelectFieldEqual<ui16> duration;
	extern DLL_EXPORT CSelectFieldEqual<ui8> sourceType;
	extern DLL_EXPORT CSelectFieldEqual<ui8> effectRange;
	extern DLL_EXPORT CWillLastTurns turns;

	CSelector DLL_EXPORT typeSubtype(TBonusType Type, TBonusSubtype Subtype);
	CSelector DLL_EXPORT typeSubtypeInfo(TBonusType type, TBonusSubtype subtype, si32 info);
	CSelector DLL_EXPORT source(ui8 source, ui32 sourceID);
	CSelector DLL_EXPORT durationType(ui16 duration);
	CSelector DLL_EXPORT sourceTypeSel(ui8 source);

	bool DLL_EXPORT matchesType(const CSelector &sel, TBonusType type);
	bool DLL_EXPORT matchesTypeSubtype(const CSelector &sel, TBonusType type, TBonusSubtype subtype);
	bool DLL_EXPORT positiveSpellEffects(const Bonus *b);
}

extern DLL_EXPORT const std::map<std::string, int> bonusNameMap;
