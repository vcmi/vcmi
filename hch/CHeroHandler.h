#ifndef CHEROHANDLER_H
#define CHEROHANDLER_H
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
	std::string bonusName, shortBonus, longBonus; //for special abilities
	std::string biography; //biography, of course
	bool isAllowed; //true if we can play with this hero (depends on map)
	CHeroClass * heroClass;
	EHeroClasses heroType; //hero class
	//bool operator<(CHero& drugi){if (ID < drugi.ID) return true; else return false;}
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
	std::vector<int> terrCosts; //default costs of going through terrains: dirt, sand, grass, snow, swamp, rough, subterrain, lava, water, rock; -1 means terrain is imapassable
	CDefHandler * moveAnim; //added group 10: up - left, 11 - left and 12 - left down // 13 - up-left standing; 14 - left standing; 15 - left down standing
	int chooseSecSkill(std::set<int> possibles); //picks secondary skill out from given possibilities
	CHeroClass();
	~CHeroClass();
};

class DLL_EXPORT CHeroHandler
{
public:
	std::vector<CHero*> heroes; //by³o nodrze
	std::vector<CHeroClass *> heroClasses;
	std::vector<std::string> pskillsn;
	std::vector<int> expPerLevel; //expPerLEvel[i] is amount of exp needed to reach level i; if it is not in this vector, multiplicate last value by 1,2 to get next value
	unsigned int level(unsigned int experience);
	unsigned int reqExp(unsigned int level);
	void loadHeroes();
	void loadSpecialAbilities();
	void loadBiographies();
	void loadHeroClasses();
	void loadPortraits(); //loads names of primary skills
	void initHeroClasses();
	~CHeroHandler();
	void initTerrainCosts();
};
#endif //CHEROHANDLER_H
