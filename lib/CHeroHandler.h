#pragma once

#include "../lib/ConstTransitivePtr.h"
#include "GameConstants.h"
#include "HeroBonus.h"
#include "IHandlerBase.h"

/*
 * CHeroHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CHeroClass;
class CDefHandler;
class CGameInfo;
class CGHeroInstance;
struct BattleHex;
class JsonNode;
class CRandomGenerator;

struct SSpecialtyInfo
{	si32 type;
	si32 val;
	si32 subtype;
	si32 additionalinfo;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type & val & subtype & additionalinfo;
	}
};

struct SSpecialtyBonus
/// temporary hold
{
	ui8 growsWithLevel;
	BonusList bonuses;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & growsWithLevel & bonuses;
	}
};

class DLL_LINKAGE CHero
{
public:
	struct InitialArmyStack
	{
		ui32 minAmount;
		ui32 maxAmount;
		CreatureID creature;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & minAmount & maxAmount & creature;
		}
	};

	HeroTypeID ID;
	si32 imageIndex;

	std::vector<InitialArmyStack> initialArmy;

	CHeroClass * heroClass;
	std::vector<std::pair<SecondarySkill, ui8> > secSkillsInit; //initial secondary skills; first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert)
	std::vector<SSpecialtyInfo> spec;
	std::vector<SSpecialtyBonus> specialty;
	std::set<SpellID> spells;
	bool haveSpellBook;
	bool special; // hero is special and won't be placed in game (unless preset on map), e.g. campaign heroes
	ui8 sex; // default sex: 0=male, 1=female

	/// Localized texts
	std::string name; //name of hero
	std::string biography;
	std::string specName;
	std::string specDescr;
	std::string specTooltip;

	/// Graphics
	std::string iconSpecSmall;
	std::string iconSpecLarge;
	std::string portraitSmall;
	std::string portraitLarge;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & imageIndex & initialArmy & heroClass & secSkillsInit & spec & specialty & spells & haveSpellBook & sex & special;
		h & name & biography & specName & specDescr & specTooltip;
		h & iconSpecSmall & iconSpecLarge & portraitSmall & portraitLarge;
	}
};

class DLL_LINKAGE CHeroClass
{
public:
	enum EClassAffinity
	{
		MIGHT,
		MAGIC
	};

	std::string identifier;
	std::string name; // translatable
	//double aggression; // not used in vcmi.
	TFaction faction;
	ui8 id;
	ui8 affinity; // affility, using EClassAffinity enum

	// default chance for hero of specific class to appear in tavern, if field "tavern" was not set
	// resulting chance = sqrt(town.chance * heroClass.chance)
	ui32 defaultTavernChance;

	CCreature * commander;

	std::vector<int> primarySkillInitial;  // initial primary skills
	std::vector<int> primarySkillLowLevel; // probability (%) of getting point of primary skill when getting level
	std::vector<int> primarySkillHighLevel;// same for high levels (> 10)

	std::vector<int> secSkillProbability; //probabilities of gaining secondary skills (out of 112), in id order

	std::map<TFaction, int> selectionProbability; //probability of selection in towns

	std::string imageBattleMale;
	std::string imageBattleFemale;
	std::string imageMapMale;
	std::string imageMapFemale;

	CHeroClass();

	bool isMagicHero() const;
	SecondarySkill chooseSecSkill(const std::set<SecondarySkill> & possibles, CRandomGenerator & rand) const; //picks secondary skill out from given possibilities

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & identifier & name & faction & id & defaultTavernChance;// & aggression;
		h & primarySkillInitial   & primarySkillLowLevel;
		h & primarySkillHighLevel & secSkillProbability;
		h & selectionProbability & affinity & commander;
		h & imageBattleMale & imageBattleFemale & imageMapMale & imageMapFemale;
	}
	EAlignment::EAlignment getAlignment() const;
};

struct DLL_LINKAGE CObstacleInfo
{
	si32 ID;
	std::string defName;
	std::vector<ETerrainType> allowedTerrains;
	std::vector<BFieldType> allowedSpecialBfields;

	ui8 isAbsoluteObstacle; //there may only one such obstacle in battle and its position is always the same
	si32 width, height; //how much space to the right and up is needed to place obstacle (affects only placement algorithm)
	std::vector<si16> blockedTiles; //offsets relative to obstacle position (that is its left bottom corner)

	std::vector<BattleHex> getBlocked(BattleHex hex) const; //returns vector of hexes blocked by obstacle when it's placed on hex 'hex'

	bool isAppropriate(ETerrainType terrainType, int specialBattlefield = -1) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & defName & allowedTerrains & allowedSpecialBfields & isAbsoluteObstacle & width & height & blockedTiles;
	}
};

class DLL_LINKAGE CHeroClassHandler : public IHandlerBase
{
	CHeroClass *loadFromJson(const JsonNode & node);
public:
	std::vector< ConstTransitivePtr<CHeroClass> > heroClasses;

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void afterLoadFinalization();

	std::vector<bool> getDefaultAllowed() const;

	~CHeroClassHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroClasses;
	}
};

class DLL_LINKAGE CHeroHandler : public IHandlerBase
{
	/// expPerLEvel[i] is amount of exp needed to reach level i;
	/// consists of 201 values. Any higher levels require experience larger that ui64 can hold
	std::vector<ui64> expPerLevel;

	/// helpers for loading to avoid huge load functions
	void loadHeroArmy(CHero * hero, const JsonNode & node);
	void loadHeroSkills(CHero * hero, const JsonNode & node);
	void loadHeroSpecialty(CHero * hero, const JsonNode & node);

	void loadExperience();
	void loadBallistics();
	void loadTerrains();
	void loadObstacles();

	/// Load single hero from json
	CHero * loadFromJson(const JsonNode & node);

public:
	CHeroClassHandler classes;

	std::vector< ConstTransitivePtr<CHero> > heroes;

	//default costs of going through terrains. -1 means terrain is impassable
	std::vector<int> terrCosts;

	struct SBallisticsLevelInfo
	{
		ui8 keep, tower, gate, wall; //chance to hit in percent (eg. 87 is 87%)
		ui8 shots; //how many shots we have
		ui8 noDmg, oneDmg, twoDmg; //chances for shot dealing certain dmg in percent (eg. 87 is 87%); must sum to 100
		ui8 sum; //I don't know if it is useful for anything, but it's in config file
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & keep & tower & gate & wall & shots & noDmg & oneDmg & twoDmg & sum;
		}
	};
	std::vector<SBallisticsLevelInfo> ballistics; //info about ballistics ability per level; [0] - none; [1] - basic; [2] - adv; [3] - expert

	std::map<int, CObstacleInfo> obstacles; //info about obstacles that may be placed on battlefield
	std::map<int, CObstacleInfo> absoluteObstacles; //info about obstacles that may be placed on battlefield

	ui32 level(ui64 experience) const; //calculates level corresponding to given experience amount
	ui64 reqExp(ui32 level) const; //calculates experience required for given level

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	CHeroHandler(); //c-tor
	~CHeroHandler(); //d-tor

	std::vector<bool> getDefaultAllowed() const;

	/**
	 * Gets a list of default allowed abilities. OH3 abilities/skills are all allowed by default.
	 *
	 * @return a list of allowed abilities, the index is the ability id
	 */
	std::vector<bool> getDefaultAllowedAbilities() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & classes & heroes & expPerLevel & ballistics & terrCosts;
		h & obstacles & absoluteObstacles;
	}
};
