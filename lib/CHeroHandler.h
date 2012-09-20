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
	enum EHeroClasses {KNIGHT, CLERIC, RANGER, DRUID, ALCHEMIST, WIZARD,
		DEMONIAC, HERETIC, DEATHKNIGHT, NECROMANCER, WARLOCK, OVERLORD,
		BARBARIAN, BATTLEMAGE, BEASTMASTER, WITCH, PLANESWALKER, ELEMENTALIST};

	std::string name; //name of hero
	si32 ID;
	ui32 lowStack[3], highStack[3]; //amount of units; described below
	std::string refTypeStack[3]; //reference names of units appearing in hero's army if he is recruited in tavern
	CHeroClass * heroClass;
	EHeroClasses heroType; //hero class
	std::vector<std::pair<ui8,ui8> > secSkillsInit; //initial secondary skills; first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert)
	std::vector<SSpecialtyInfo> spec;
	si32 startingSpell; //-1 if none
	ui8 sex; // default sex: 0=male, 1=female
	//bool operator<(CHero& drugi){if (ID < drugi.ID) return true; else return false;}

	CHero();
	~CHero();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & ID & lowStack & highStack & refTypeStack	& heroClass & heroType & secSkillsInit & spec & startingSpell & sex;
	}
};

class DLL_LINKAGE CHeroClass
{
public:
	ui8 alignment; 
	ui32 skillLimit; //how many secondary skills can hero learn
	std::string name;
	double aggression;
	int initialAttack, initialDefence, initialPower, initialKnowledge; //initial values of primary skills
	std::vector<std::pair<int,int> > primChance;//primChance[PRIMARY_SKILL_ID] - first is for levels 2 - 9, second for 10+;;; probability (%) of getting point of primary skill when getting new level
	std::vector<int> proSec; //probabilities of gaining secondary skills (out of 112), in id order
	int selectionProbability[9]; //probability of selection in towns

	int chooseSecSkill(const std::set<int> & possibles) const; //picks secondary skill out from given possibilities
	CHeroClass(); //c-tor
	~CHeroClass(); //d-tor

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & skillLimit & name & aggression & initialAttack & initialDefence & initialPower & initialKnowledge & primChance
			& proSec & selectionProbability & alignment;
	}
	EAlignment::EAlignment getAlignment();
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

class DLL_LINKAGE CHeroHandler
{
public:
	std::vector< ConstTransitivePtr<CHero> > heroes; //changed from nodrze
	std::vector<CHeroClass *> heroClasses;
	std::vector<ui64> expPerLevel; //expPerLEvel[i] is amount of exp needed to reach level i; if it is not in this vector, multiplicate last value by 1,2 to get next value

	 //default costs of going through terrains: dirt, sand, grass, snow, swamp, rough, subterranean, lava, water, rock; -1 means terrain is imapassable
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

	void loadObstacles(); //loads info about obstacles

	ui32 level(ui64 experience) const; //calculates level corresponding to given experience amount
	ui64 reqExp(ui32 level) const; //calculates experience required for given level

	void loadHeroes();
	void loadHeroClasses();
	void initHeroClasses();
	void loadTerrains();
	CHeroHandler(); //c-tor
	~CHeroHandler(); //d-tor

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroClasses & heroes & expPerLevel & ballistics & terrCosts;
		h & obstacles & absoluteObstacles;
		if(!h.saving)
		{
			//restore class pointers
			for (int i=0; i<heroes.size(); i++)
			{
				heroes[i]->heroClass = heroClasses[heroes[i]->heroType];
			}
		}
	}
};
