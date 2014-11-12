#pragma once

#include "IHandlerBase.h"
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
	struct LevelInfo
	{
		std::string description; //descriptions of spell for skill level
		si32 cost; //per skill level: 0 - none, 1 - basic, etc
		si32 power; //per skill level: 0 - none, 1 - basic, etc
		si32 AIValue; //AI values: per skill level: 0 - none, 1 - basic, etc

		bool smartTarget;
		std::string range;

		std::vector<Bonus> effects;

		LevelInfo();
		~LevelInfo();

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & description & cost & power & AIValue & smartTarget & range & effects;
		}
	};

	/** \brief Low level accessor. Don`t use it if absolutely necessary
	 *
	 * \param level. spell school level
	 * \return Spell level info structure
	 *
	 */
	const CSpell::LevelInfo& getLevelInfo(const int level) const;
public:
	enum ETargetType {NO_TARGET, CREATURE, OBSTACLE};
	enum ESpellPositiveness {NEGATIVE = -1, NEUTRAL = 0, POSITIVE = 1};

	struct TargetInfo
	{
		ETargetType type;
		bool smart;
		bool massive;
		bool onlyAlive;
	};

	SpellID id;
	std::string identifier; //???
	std::string name;

	si32 level;
	bool earth;
	bool water;
	bool fire;
	bool air;
	si32 power; //spell's power

	std::map<TFaction, si32> probabilities; //% chance to gain for castles

	bool combatSpell; //is this spell combat (true) or adventure (false)
	bool creatureAbility; //if true, only creatures can use this spell
	si8 positiveness; //1 if spell is positive for influenced stacks, 0 if it is indifferent, -1 if it's negative

	std::vector<SpellID> counteredSpells; //spells that are removed when effect of this spell is placed on creature (for bless-curse, haste-slow, and similar pairs)

	CSpell();
	~CSpell();

	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes = nullptr ) const; //convert range to specific hexes; last optional out parameter is set to true, if spell would cover unavailable hexes (that are not included in ret)
	si16 mainEffectAnim; //main spell effect animation, in AC format (or -1 when none)
	ETargetType getTargetType() const; //deprecated

	const CSpell::TargetInfo getTargetInfo(const int level) const;

	bool isCombatSpell() const;
	bool isAdventureSpell() const;
	bool isCreatureAbility() const;

	bool isPositive() const;
	bool isNegative() const;
	bool isNeutral() const;

	bool isRisingSpell() const;
	bool isDamageSpell() const;
	bool isOffensiveSpell() const;

	bool isSpecialSpell() const;

	bool hasEffects() const;
	void getEffects(std::vector<Bonus> &lst, const int level) const;

	bool isImmuneBy(const IBonusBearer *obj) const;

	si32 getCost(const int skillLevel) const;

	/**
	 * Returns spell level power, base power ignored
	 */
	si32 getPower(const int skillLevel) const;

//	/**
//	* Returns spell power, taking base power into account
//	*/
//	si32 calculatePower(const int skillLevel) const;


	si32 getProbability(const TFaction factionId) const;

	/**
	 * Returns resource name of icon for SPELL_IMMUNITY bonus
	 */
	const std::string& getIconImmune() const;

	const std::string& getCastSound() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & identifier & id & name & level & earth & water & fire & air & power
		  & probabilities  & attributes & combatSpell & creatureAbility & positiveness & counteredSpells & mainEffectAnim;
		h & isRising & isDamage & isOffensive;
		h & targetType;
		h & immunities & limiters & absoluteImmunities & absoluteLimiters;
		h & iconImmune;
		h & defaultProbability;

		h & isSpecial;

		h & castSound & iconBook & iconEffect & iconScenarioBonus & iconScroll;

		h & levels;

	}
	friend class CSpellHandler;
	friend class Graphics;

private:
	void setIsOffensive(const bool val);
	void setIsRising(const bool val);

private:
	si32 defaultProbability;

	bool isRising;
	bool isDamage;
	bool isOffensive;
	bool isSpecial;

	std::string attributes; //reference only attributes //todo: remove or include in configuration format, currently unused

	ETargetType targetType;

	std::vector<Bonus::BonusType> immunities; //any of these grants immunity
	std::vector<Bonus::BonusType> absoluteImmunities; //any of these grants immunity, can't be negated
	std::vector<Bonus::BonusType> limiters; //all of them are required to be affected
	std::vector<Bonus::BonusType> absoluteLimiters; //all of them are required to be affected, can't be negated

	///graphics related stuff

	std::string iconImmune;

	std::string iconBook;
	std::string iconEffect;
	std::string iconScenarioBonus;
	std::string iconScroll;

	///sound related stuff
	std::string castSound;

	std::vector<LevelInfo> levels;
};



bool DLL_LINKAGE isInScreenRange(const int3 &center, const int3 &pos); //for spells like Dimension Door

class DLL_LINKAGE CSpellHandler: public CHandlerBase<SpellID, CSpell>
{
public:
	CSpellHandler();
	virtual ~CSpellHandler();

	///IHandler base
	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
	void afterLoadFinalization() override;
	void beforeValidate(JsonNode & object) override;

	/**
	 * Gets a list of default allowed spells. OH3 spells are all allowed by default.
	 *
	 * @return a list of allowed spells, the index is the spell id and the value either 0 for not allowed or 1 for allowed
	 */
	std::vector<bool> getDefaultAllowed() const override;

	const std::string getTypeName() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects ;
	}
protected:
	CSpell * loadFromJson(const JsonNode & json) override;
};
