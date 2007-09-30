#ifndef CHEROHANDLER_H
#define CHEROHANDLER_H

#include <string>
#include <vector>
#include "CCreatureHandler.h"
#include "SDL.h"
#include "../int3.h"
#include "CAmbarCendamo.h"
#include "CGameInterface.h"

class CHeroClass;
class CObjectInstance;
class CDefHandler;
class CGameInfo;

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
	std::vector<int> terrCosts; //default costs of going through terrains: dirt, sand, grass, snow, swamp, rough, subterrain, lava, water, rock; -1 means terrain is imapassable
	CDefHandler * moveAnim; //added group 10: up - left, 11 - left and 12 - left down
};

class CHeroInstance
{
public:
	int owner;
	CHero * type;
	CObjectInstance * ourObject;
	int exp; //experience point
	int level; //current level of hero
	std::string name; //may be custom
	std::string biography; //may be custom
	int portrait; //may be custom
	int3 pos; //position on adventure map
	CCreatureSet army; //army
	int mana; // remaining spell points
	std::vector<int> primSkills; //0-attack, 1-defence, 2-spell power, 3-knowledge
	std::vector<std::pair<int,int> > secSkills; //first - ID of skill, second - level of skill (0 - basic, 1 - adv., 2 - expert)
	int movement; //remaining movement points
	bool inTownGarrison; // if hero is in town garrison 

	unsigned int getTileCost(EterrainType & ttype, Eroad & rdtype, Eriver & rvtype);
	unsigned int getLowestCreatureSpeed();
	unsigned int getAdditiveMoveBonus();
	unsigned float getMultiplicativeMoveBonus();

	//TODO: artifacts, known spells, commander, blessings, curses, morale/luck special modifiers
};

class CHeroHandler
{
public:
	std::vector<CHeroInstance *> heroInstances;
	std::vector<CHero*> heroes; //by³o nodrze
	std::vector<CHeroClass *> heroClasses;
	unsigned int level(unsigned int experience);
	void loadHeroes();
	void loadSpecialAbilities();
	void loadBiographies();
	void loadHeroClasses();
	void loadPortraits();
	void initHeroClasses();
	~CHeroHandler();
	void initTerrainCosts();

	friend void CAmbarCendamo::deh3m();
	friend void initGameState(CGameInfo * cgi);
	friend class CConsoleHandler;

	//friend void CPlayerInterface::heroMoved(const HeroMoveDetails & details); //TODO: wywalic, wstretne!!!
};


#endif //CHEROHANDLER_H