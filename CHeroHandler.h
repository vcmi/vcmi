#ifndef CHEROHANDLER_H
#define CHEROHANDLER_H

#include <string>
#include <vector>
#include "CCreatureHandler.h"
#include "SDL.h"
#include "int3.h"

class CHeroClass;


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
	int owner;
	int exp; //experience point
	int level; //current level of hero
	std::string name; //may be custom
	std::string biography; //may be custom
	int portrait; //may be custom
	CHero type;
	int3 pos; //position on adventure map
	CCreatureSet army; //army
	int mana; // remaining spell points
	int movement; //remaining movement points
	//TODO: artifacts, primary and secondary skills, known spells, commander, blessings, curses, morale/luck modifiers
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
	void loadPortraits();
	void initHeroClasses();
	~CHeroHandler();
};


#endif //CHEROHANDLER_H