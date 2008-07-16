#ifndef CTOWNHANDLER_H
#define CTOWNHANDLER_H

#include "CDefHandler.h"
#include "CCreatureHandler.h"
#include "SDL.h"
#include "../int3.h"
#include <string>
#include <vector>
#include <set>
class CBuilding;
class CSpell;
class CHero;
class CGTownInstance;
class CTown
{
public:
	std::string name; //name of type
	std::vector<std::string> names; //names of the town instances
	std::vector<int> basicCreatures; //level (from 0) -> ID
	std::vector<int> upgradedCreatures; //level (from 0) -> ID
	std::map<int,int> hordeLvl; //[0] - first horde building creature level; [1] - second horde building (-1 if not present)
	int bonus; //pic number
	int typeID;
};

struct Structure
{
	int ID;
	int3 pos;
	std::string defName, borderName, areaName, name;
	int townID, group;
	bool operator<(const Structure & p2) const
	{
		if(pos.z != p2.pos.z)
			return (pos.z) < (p2.pos.z);
		else
			return (ID) < (p2.ID);
	}
};

class CTownHandler
{
public:
	CDefHandler * smallIcons, *resources; //resources 32x32
	CTownHandler();
	~CTownHandler();
	std::vector<CTown> towns;
	std::vector<std::string> tcommands, hcommands;
	void loadNames();
	SDL_Surface * getPic(int ID, bool fort=true, bool builded=false); //ID=-1 - blank; -2 - border; -3 - random
	static int getTypeByDefName(std::string name);

	std::map<int,std::map<int, Structure*> > structures; // <town ID, <structure ID, structure>>
	std::map<int, std::map<int,std::set<int> > > requirements; //requirements[town_id][structure_id] -> set of required buildings

	std::vector<CGTownInstance *> townInstances;

};

#endif //CTOWNHANDLER_H
