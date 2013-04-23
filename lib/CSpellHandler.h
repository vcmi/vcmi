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
	SpellID id;
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

	bool combatSpell; //is this spell combat (true) or adventure (false)
	bool creatureAbility; //if true, only creatures can use this spell
	si8 positiveness; //1 if spell is positive for influenced stacks, 0 if it is indifferent, -1 if it's negative
	std::vector<std::string> range; //description of spell's range in SRSL by magic school level
	std::vector<SpellID> counteredSpells; //spells that are removed when effect of this spell is placed on creature (for bless-curse, haste-slow, and similar pairs)

	CSpell();
	~CSpell();

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
	inline bool isOffensiveSpell() const;

	inline bool hasEffects() const;
	void getEffects(std::vector<Bonus> &lst, const int level) const;

	bool isImmuneBy(const IBonusBearer *obj) const;

	/**
	* Returns resource name of icon for SPELL_IMMUNITY bonus
	*/
	inline const std::string& getIconImmune() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & identifier & id & name & abbName & descriptions & level & earth & water & fire & air & power & costs
			& powers & probabilities & AIVals & attributes & combatSpell & creatureAbility & positiveness & range & counteredSpells & mainEffectAnim;
		h & isRising & isDamage & isOffensive;
		h & targetType;
		h & effects & immunities & limiters;
		h & iconImmune;
	}
	friend class CSpellHandler;

private:


	bool isRising;
	bool isDamage;
	bool isOffensive;

	std::string attributes; //reference only attributes

	void setAttributes(const std::string& newValue);

	ETargetType targetType;

	std::vector<std::vector<Bonus *> > effects; // [level 0-3][list of effects]
	std::vector<Bonus::BonusType> immunities; //any of these grants immunity
	std::vector<Bonus::BonusType> limiters; //all of them are required to be affected

	///graphics related stuff

	std::string iconImmune;
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

bool CSpell::isOffensiveSpell() const
{
	return isOffensive;
}

bool CSpell::hasEffects() const
{
	return effects.size();
}

const std::string& CSpell::getIconImmune() const
{
	return iconImmune;
}


bool DLL_LINKAGE isInScreenRange(const int3 &center, const int3 &pos); //for spells like Dimension Door

class DLL_LINKAGE CSpellHandler
{
	CSpell * loadSpell(CLegacyConfigParser & parser, const SpellID id);

public:
	CSpellHandler();
	~CSpellHandler();
	std::vector< ConstTransitivePtr<CSpell> > spells;

	/**
	 * Gets a list of default allowed spells. OH3 spells are all allowed by default.
	 *
	 * @return a list of allowed spells, the index is the spell id and the value either 0 for not allowed or 1 for allowed
	 */
	std::vector<bool> getDefaultAllowed() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & spells ;
	}
};
