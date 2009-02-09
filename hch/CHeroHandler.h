#ifndef __CHEROHANDLER_H__
#define __CHEROHANDLER_H__
#include "../global.h"
#include <string>
#include <vector>
#include <set>
class CHeroClass;
class CDefHandler;
class CGameInfo;
class CGHeroInstance;
class DLL_EXPORT CHero
{
public:
	std::string name;
	int ID;
	int lowStack[3], highStack[3]; //amount of units; described below
	std::string refTypeStack[3]; //reference names of units appearing in hero's army if he is recruited in tavern
	CHeroClass * heroClass;
	EHeroClasses heroType; //hero class
	std::vector<std::pair<ui8,ui8> > secSkillsInit; //initial secondary skills; first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert)
	//bool operator<(CHero& drugi){if (ID < drugi.ID) return true; else return false;}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & ID & lowStack & highStack & refTypeStack	& heroType & ID;
		//hero class pointer is restored by herohandler
	}
};

class DLL_EXPORT CHeroClass
{
public:
	ui32 skillLimit; //how many secondary skills can hero learn
	std::string name;
	float aggression;
	int initialAttack, initialDefence, initialPower, initialKnowledge;
	std::vector<std::pair<int,int> > primChance;//primChance[PRIMARY_SKILL_ID] - first is for levels 2 - 9, second for 10+;;; probability (%) of getting point of primary skill when getting new level
	std::vector<int> proSec; //probabilities of gaining secondary skills (out of 112), in id order
	int selectionProbability[9]; //probability of selection in towns
	std::vector<int> terrCosts; //default costs of going through terrains: dirt, sand, grass, snow, swamp, rough, subterranean, lava, water, rock; -1 means terrain is imapassable

	int chooseSecSkill(const std::set<int> & possibles) const; //picks secondary skill out from given possibilities
	CHeroClass();
	~CHeroClass();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & skillLimit & name & aggression & initialAttack & initialDefence & initialPower & initialKnowledge & primChance
			& proSec & selectionProbability & terrCosts;
	}
};

struct DLL_EXPORT CObstacleInfo
{
	int ID;
	std::string defName, 
		blockmap, //blockmap: X - blocked, N - not blocked, L - description goes to the next line, staring with the left bottom hex
		allowedTerrains; /*terrains[i]: 1. sand/shore   2. sand/mesas   3. dirt/birches   4. dirt/hills   5. dirt/pines   6. grass/hills   
			7. grass/pines   8. lava   9. magic plains   10. snow/mountains   11. snow/trees   12. subterranean   13. swamp/trees  
			14. fiery fields   15. rock lands   16. magic clouds   17. lucid pools   18. holy ground   19. clover field  
			20. evil fog   21. "favourable winds" text on magic plains background   22. cursed ground   23. rough 
			24. ship to ship   25. ship*/
	int getWidth(); //returns width of obstacle in hexes
	int getHeight(); //returns height of obstacle in hexes
	std::vector<int> getBlocked(int hex); //returns vector of hexes blocked by obstacle when it's placed on hex 'hex'
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & defName & blockmap & allowedTerrains;
	}
};

class DLL_EXPORT CHeroHandler
{
public:
	std::vector<CHero*> heroes; //by³o nodrze
	std::vector<CHeroClass *> heroClasses;
	std::vector<int> expPerLevel; //expPerLEvel[i] is amount of exp needed to reach level i; if it is not in this vector, multiplicate last value by 1,2 to get next value
	
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

	void loadObstacles();

	unsigned int level(unsigned int experience);
	unsigned int reqExp(unsigned int level);

	void loadHeroes();
	void loadHeroClasses();
	void initHeroClasses();
	void initTerrainCosts();
	CHeroHandler();
	~CHeroHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroClasses & heroes & expPerLevel & ballistics & obstacles;
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

#endif // __CHEROHANDLER_H__
