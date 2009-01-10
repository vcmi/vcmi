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

class DLL_EXPORT CHeroHandler
{
public:
	std::vector<CHero*> heroes; //by³o nodrze
	std::vector<CHeroClass *> heroClasses;
	std::vector<int> expPerLevel; //expPerLEvel[i] is amount of exp needed to reach level i; if it is not in this vector, multiplicate last value by 1,2 to get next value

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
		h & heroClasses & heroes & expPerLevel;
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
