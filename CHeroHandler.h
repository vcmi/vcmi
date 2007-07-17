#ifndef CHEROHANDLER_H
#define CHEROHANDLER_H

#include <string>
#include <vector>
#include "CCreatureHandler.h"
#include "SDL.h"

class CHeroClass;

enum EHeroClasses {HERO_KNIGHT, HERO_CLERIC, HERO_RANGER, HERO_DRUID, HREO_ALCHEMIST, HERO_WIZARD, HERO_DEMONIAC, HERO_HERETIC, HERO_DEATHKNIGHT, HERO_NECROMANCER, HERO_WARLOCK, HERO_OVERLORD, HERO_BARBARIAN, HERO_BATTLEMAGE, HERO_BEASTMASTER, HERO_WITCH, HERO_PLANESWALKER, HERO_ELEMENTALIST};

class CHero
{
public:
	std::string name;
	int ID;
	int low1stack, high1stack, low2stack, high2stack, low3stack, high3stack; //amount of units; described below
	std::string refType1stack, refType2stack, refType3stack; //reference names of units appearing in hero's army if he is recruited in tavern
	std::string bonusName, shortBonus, longBonus; //for special abilities
	std::string biography; //biography, of course
	bool isAllowed; //true if we can play with this hero (depends on map)
	CHeroClass * heroClass;
	EHeroClasses heroType; //hero class
	//bool operator<(CHero& drugi){if (ID < drugi.ID) return true; else return false;}
	SDL_Surface * portraitSmall; //48x32 p
};

class CHeroClass
{
public:
	std::string name;
	float aggression;
	int initialAttack, initialDefence, initialPower, initialKnowledge;
	int proAttack[2]; //probability of gaining attack point on levels [0]: 2 - 9; [1]: 10+  (out of 100)
	int proDefence[2]; //probability of gaining defence point on levels [0]: 2 - 9; [1]: 10+ (out of 100)
	int proPower[2]; //probability of gaining power point on levels [0]: 2 - 9; [1]: 10+ (out of 100)
	int proKnowledge[2]; //probability of gaining knowledge point on levels [0]: 2 - 9; [1]: 10+ (out of 100)
	std::vector<int> proSec; //probabilities of gaining secondary skills (out of 112), in id order
	int selectionProbability[9]; //probability of selection in towns
};

class CHeroInstance
{
public:
	CHero type;
	int x, y, z; //position
	CCreatureSet army; //army
	//TODO: armia, artefakty, itd.
};

class CHeroHandler
{
public:
	std::vector<CHero*> heroes; //by³o nodrze
	std::vector<CHeroClass *> heroClasses;
	void loadHeroes();
	void loadSpecialAbilities();
	void loadBiographies();
	void loadHeroClasses();
	void initHeroClasses();
};


#endif //CHEROHANDLER_H