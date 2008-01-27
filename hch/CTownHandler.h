#ifndef CTOWNHANDLER_H
#define CTOWNHANDLER_H

#include "CDefHandler.h"
#include "CCreatureHandler.h"
#include "SDL.h"
#include "../int3.h"
#include <string>
#include <vector>

class CBuilding;
class CSpell;
class CHero;
class CTownInstance;
class CGTownInstance;
class CHeroInstance;
class CTown
{
public:
	std::string name; //name of type
	std::vector<std::string> names; //names of the town instances
	int bonus; //pic number
	int typeID;
};

struct Structure
{
	int ID;
	int3 pos;
	std::string defName, borderName, areaName, name;
	int townID, group;
};

class CTownHandler
{
	CDefHandler * smallIcons;
public:
	CTownHandler();
	~CTownHandler();
	std::vector<CTown> towns;
	std::vector<std::string> tcommands;
	void loadNames();
	SDL_Surface * getPic(int ID, bool fort=true, bool builded=false); //ID=-1 - blank; -2 - border; -3 - random
	static int getTypeByDefName(std::string name);

	std::map<int,std::map<int, Structure*> > structures; // <town ID, <structure ID, structure>>


	std::vector<CGTownInstance *> townInstances;

};

class CTownInstance
{
public:
	int3 pos; //position
	CTown * town;
	std::string name; // name of town
	CCreatureSet garrison;
	int builded; //how many buildings has been built this turn
	int destroyed; //how many buildings has been destroyed this turn
	
	//TODO:
	std::vector<CBuilding *> /*allBuildings,*/ possibleBuildings, builtBuildings;
	std::vector<int> creatureIncome; //vector by level
	std::vector<int> creaturesLeft; //that can be recruited

	CHeroInstance * garrisonHero;

	std::vector<CSpell *> possibleSpells, obligatorySpells, availableSpells;

	int getSightDistance() const; //returns sight distance

	CTownInstance();
};

#endif //CTOWNHANDLER_H