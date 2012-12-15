#pragma once


#include "../lib/ConstTransitivePtr.h"
#include "GameConstants.h"

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

class DLL_LINKAGE CHero
{
public:
	struct InitialArmyStack
	{
		ui32 minAmount;
		ui32 maxAmount;
		TCreature creature;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & minAmount & maxAmount & creature;
		}
	};

	std::string name; //name of hero
	si32 ID;

	InitialArmyStack initialArmy[3];

	CHeroClass * heroClass;
	std::vector<std::pair<ui8,ui8> > secSkillsInit; //initial secondary skills; first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert)
	std::vector<SSpecialtyInfo> spec;
	si32 startingSpell; //-1 if none
	ui8 sex; // default sex: 0=male, 1=female

	CHero();
	~CHero();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & ID & initialArmy & heroClass & secSkillsInit & spec & startingSpell & sex;
	}
};

class DLL_LINKAGE CHeroClass
{
public:
	std::string identifier;
	std::string name; // translatable
	double aggression;
	TFaction faction;
	ui8 id;

	std::vector<int> primarySkillInitial;  // initial primary skills
	std::vector<int> primarySkillLowLevel; // probability (%) of getting point of primary skill when getting level
	std::vector<int> primarySkillHighLevel;// same for high levels (> 10)

	std::vector<int> secSkillProbability; //probabilities of gaining secondary skills (out of 112), in id order

	std::map<TFaction, int> selectionProbability; //probability of selection in towns

	int chooseSecSkill(const std::set<int> & possibles) const; //picks secondary skill out from given possibilities
	CHeroClass(); //c-tor
	~CHeroClass(); //d-tor

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & identifier & name & faction & aggression;
		h & primarySkillInitial   & primarySkillLowLevel;
		h & primarySkillHighLevel & secSkillProbability;
		h & selectionProbability;
	}
	EAlignment::EAlignment getAlignment() const;
};

struct DLL_LINKAGE CObstacleInfo
{
	si32 ID;
	std::string defName;
	std::vector<ui8> allowedTerrains;
	std::vector<ui8> allowedSpecialBfields;

	ui8 isAbsoluteObstacle; //there may only one such obstacle in battle and its position is always the same
	si32 width, height; //how much space to the right and up is needed to place obstacle (affects only placement algorithm)
	std::vector<si16> blockedTiles; //offsets relative to obstacle position (that is its left bottom corner)

	std::vector<BattleHex> getBlocked(BattleHex hex) const; //returns vector of hexes blocked by obstacle when it's placed on hex 'hex'

	bool isAppropriate(int terrainType, int specialBattlefield = -1) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & defName & allowedTerrains & allowedSpecialBfields & isAbsoluteObstacle & width & height & blockedTiles;
	}
};

class DLL_LINKAGE CHeroClassHandler
{
public:
	std::vector< ConstTransitivePtr<CHeroClass> > heroClasses;

	/// load from H3 config
	void load();

	/// load any number of classes from json
	void load(const JsonNode & classes);

	/// load one class from json
	CHeroClass * loadClass(const JsonNode & heroClass);

	~CHeroClassHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroClasses;
	}
};

class DLL_LINKAGE CHeroHandler
{
	/// expPerLEvel[i] is amount of exp needed to reach level i;
	/// consists of 201 values. Any higher levels require experience larger that ui64 can hold
	std::vector<ui64> expPerLevel;

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

	/// Load multiple heroes from json
	void load(const JsonNode & heroes);

	/// Load single hero from json
	CHero * loadHero(const JsonNode & hero);

	/// Load everything (calls functions below + classes.load())
	void load();

	void loadHeroes();
	void loadExperience();
	void loadBallistics();
	void loadTerrains();
	void loadObstacles();

	CHeroHandler(); //c-tor
	~CHeroHandler(); //d-tor

	/**
	 * Gets a list of default allowed heroes.
	 *
	 * TODO Proposal for hero modding: Replace hero id with a unique machine readable hero name and
	 * create a JSON config file or merge it with a existing config file which describes which heroes can be used for
	 * random map generation / map editor(default map settings). (Gelu, ... should be excluded)
	 *
	 * @return a list of allowed heroes, the index is the hero id and the value either 0 for not allowed and 1 for allowed
	 */
	std::vector<ui8> getDefaultAllowedHeroes() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & classes & heroes & expPerLevel & ballistics & terrCosts;
		h & obstacles & absoluteObstacles;
	}
};
