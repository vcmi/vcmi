#pragma once

#include "../lib/ConstTransitivePtr.h"
#include "int3.h"
#include "GameConstants.h"
#include "HeroBonus.h"

/*
 * CSpellHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CLegacyConfigParser;
struct BattleHex;

class DLL_LINKAGE CSpell
{
public:
	enum ETargetType {NO_TARGET, CREATURE, CREATURE_EXPERT_MASSIVE, OBSTACLE};
	enum ESpellPositiveness {NEGATIVE = -1, NEUTRAL = 0, POSITIVE = 1};
	TSpell id;
	std::string identifier;
	std::string name;
	std::string abbName; //abbreviated name
	std::vector<std::string> descriptions; //descriptions of spell for skill levels: 0 - none, 1 - basic, etc
	si32 level;
	bool earth;
	bool water;
	bool fire;
	bool air;
	si32 power; //spell's power
	std::vector<si32> costs; //per skill level: 0 - none, 1 - basic, etc
	std::vector<si32> powers; //[er skill level: 0 - none, 1 - basic, etc
	std::map<TFaction, si32> probabilities; //% chance to gain for castles
	std::vector<si32> AIVals; //AI values: per skill level: 0 - none, 1 - basic, etc
	std::string attributes; //reference only attributes
	bool combatSpell; //is this spell combat (true) or adventure (false)
	bool creatureAbility; //if true, only creatures can use this spell
	si8 positiveness; //1 if spell is positive for influenced stacks, 0 if it is indifferent, -1 if it's negative
	std::vector<std::string> range; //description of spell's range in SRSL by magic school level
	std::vector<TSpell> counteredSpells; //spells that are removed when effect of this spell is placed on creature (for bless-curse, haste-slow, and similar pairs)

	CSpell();

	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes = NULL ) const; //convert range to specific hexes; last optional out parameter is set to true, if spell would cover unavailable hexes (that are not included in ret)
	si16 mainEffectAnim; //main spell effect animation, in AC format (or -1 when none)
	ETargetType getTargetType() const;

	inline bool isCombatSpell() const;
	inline bool isAdventureSpell() const;
	inline bool isCreatureAbility() const;

	inline bool isPositive() const;
	inline bool isNegative() const;

	inline bool isRisingSpell() const;
	inline bool isDamageSpell() const;
	inline bool isMindSpell() const; //TODO: deprecated - remove, refactor
	inline bool isOffensiveSpell() const;

	inline bool hasEffects() const;
	void getEffects(std::vector<Bonus> &lst, const int level) const;

	bool isImmuneBy(const IBonusBearer *obj) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & identifier & id & name & abbName & descriptions & level & earth & water & fire & air & power & costs
			& powers & probabilities & AIVals & attributes & combatSpell & creatureAbility & positiveness & range & counteredSpells & mainEffectAnim;
		h & isRising & isDamage & isMind;
		h & effects & immunities & limiters;
	}
	friend class CSpellHandler;

private:
	bool isRising;
	bool isDamage;
	bool isMind;
	bool isOffensive;

	std::vector<Bonus> effects [4];
	std::vector<Bonus::BonusType> immunities; //any of these hrants immunity
	std::vector<Bonus::BonusType> limiters; //all of them are required


};

///CSpell inlines

bool CSpell::isCombatSpell() const
{
	return combatSpell;
}

bool CSpell::isAdventureSpell() const
{
	return !combatSpell;
}

bool CSpell::isCreatureAbility() const
{
	return creatureAbility;
}

bool CSpell::isPositive() const
{
	return positiveness == POSITIVE;
}

bool CSpell::isNegative() const
{
	return positiveness == NEGATIVE;
}

bool CSpell::isRisingSpell() const
{
	return isRising;
}

bool CSpell::isDamageSpell() const
{
	return isDamage;
}

bool CSpell::isMindSpell() const
{
	return isMind;
}

bool CSpell::isOffensiveSpell() const
{
	return isOffensive;
}

bool CSpell::hasEffects() const
{
	return !effects[0].empty();
}


namespace Spells
{
	enum
	{
		SUMMON_BOAT=0, SCUTTLE_BOAT=1, VISIONS=2, VIEW_EARTH=3, DISGUISE=4, VIEW_AIR=5,
		FLY=6, WATER_WALK=7, DIMENSION_DOOR=8, TOWN_PORTAL=9,

		QUICKSAND=10, LAND_MINE=11, FORCE_FIELD=12, FIRE_WALL=13, EARTHQUAKE=14,
		MAGIC_ARROW=15, ICE_BOLT=16, LIGHTNING_BOLT=17, IMPLOSION=18,
		CHAIN_LIGHTNING=19, FROST_RING=20, FIREBALL=21, INFERNO=22,
		METEOR_SHOWER=23, DEATH_RIPPLE=24, DESTROY_UNDEAD=25, ARMAGEDDON=26,
		SHIELD=27, AIR_SHIELD=28, FIRE_SHIELD=29, PROTECTION_FROM_AIR=30,
		PROTECTION_FROM_FIRE=31, PROTECTION_FROM_WATER=32,
		PROTECTION_FROM_EARTH=33, ANTI_MAGIC=34, DISPEL=35, MAGIC_MIRROR=36,
		CURE=37, RESURRECTION=38, ANIMATE_DEAD=39, SACRIFICE=40, BLESS=41,
		CURSE=42, BLOODLUST=43, PRECISION=44, WEAKNESS=45, STONE_SKIN=46,
		DISRUPTING_RAY=47, PRAYER=48, MIRTH=49, SORROW=50, FORTUNE=51,
		MISFORTUNE=52, HASTE=53, SLOW=54, SLAYER=55, FRENZY=56,
		TITANS_LIGHTNING_BOLT=57, COUNTERSTRIKE=58, BERSERK=59, HYPNOTIZE=60,
		FORGETFULNESS=61, BLIND=62, TELEPORT=63, REMOVE_OBSTACLE=64, CLONE=65,
		SUMMON_FIRE_ELEMENTAL=66, SUMMON_EARTH_ELEMENTAL=67, SUMMON_WATER_ELEMENTAL=68, SUMMON_AIR_ELEMENTAL=69,

		STONE_GAZE=70, POISON=71, BIND=72, DISEASE=73, PARALYZE=74, AGE=75, DEATH_CLOUD=76, THUNDERBOLT=77,
		DISPEL_HELPFUL_SPELLS=78, DEATH_STARE=79, ACID_BREATH_DEFENSE=80, ACID_BREATH_DAMAGE=81
	};
}

bool DLL_LINKAGE isInScreenRange(const int3 &center, const int3 &pos); //for spells like Dimension Door

class DLL_LINKAGE CSpellHandler
{
	CSpell * loadSpell(CLegacyConfigParser & parser);

public:
	CSpellHandler();
	std::vector< ConstTransitivePtr<CSpell> > spells;

	void loadSpells();

	/**
	 * Gets a list of default allowed spells. OH3 spells are all allowed by default.
	 *
	 * @return a list of allowed spells, the index is the spell id and the value either 0 for not allowed or 1 for allowed
	 */
	std::vector<bool> getDefaultAllowedSpells() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & spells ;
	}
};
