#ifndef CHEROHANDLER_H
#define CHEROHANDLER_H

#include <string>
#include <vector>
#include "CCreatureHandler.h"
#include "SDL.h"
#include "../int3.h"
#include "CAmbarCendamo.h"
#include "../CGameInterface.h"

class CHeroClass;
class CDefHandler;
class CGameInfo;
class CGHeroInstance;
class CHero
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
	SDL_Surface * portraitSmall; //48x32 px
	SDL_Surface * portraitLarge; //58x64 px
};

class CHeroClass
{
public:
	std::string name;
	float aggression;
	int initialAttack, initialDefence, initialPower, initialKnowledge;
	std::vector<std::pair<int,int> > primChance;//primChance[PRIMARY_SKILL_ID] - first is for levels 2 - 9, second for 10+;;; probability (%) of getting point of primary skill when getting new level
	std::vector<int> proSec; //probabilities of gaining secondary skills (out of 112), in id order
	int selectionProbability[9]; //probability of selection in towns
	std::vector<int> terrCosts; //default costs of going through terrains: dirt, sand, grass, snow, swamp, rough, subterrain, lava, water, rock; -1 means terrain is imapassable
	CDefHandler * moveAnim; //added group 10: up - left, 11 - left and 12 - left down // 13 - up-left standing; 14 - left standing; 15 - left down standing
};

class CHeroHandler
{
public:
	std::vector<CGHeroInstance *> heroInstances;
	std::vector<CHero*> heroes; //by³o nodrze
	std::vector<CHeroClass *> heroClasses;
	std::vector<CDefHandler *> flags1, flags2, flags3, flags4; //flags blitted on heroes when ,
	CDefHandler * pskillsb, *resources; //82x93
	CDefHandler * un44; //many things
	std::vector<std::string> pskillsn;
	std::vector<int> expPerLevel; //expPerLEvel[i] is amount of exp needed to reach level i; if it is not in this vector, multiplicate last value by 1,2 to get next value
	unsigned int level(unsigned int experience);
	unsigned int reqExp(unsigned int level);
	void loadHeroes();
	void loadSpecialAbilities();
	void loadBiographies();
	void loadHeroClasses();
	void loadPortraits(); //loads also imgs and names of primary skills
	void initHeroClasses();
	~CHeroHandler();
	void initTerrainCosts();

	friend void CAmbarCendamo::deh3m();
	friend void initGameState(CGameInfo * cgi);
	//friend class CConsoleHandler;

	//friend void CPlayerInterface::heroMoved(const HeroMoveDetails & details); //TODO: wywalic, wstretne!!!
};


#endif //CHEROHANDLER_H
