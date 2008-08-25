#ifndef CTOWNHANDLER_H
#define CTOWNHANDLER_H
#include "../global.h"
#include <set>
class CBuilding;
class CSpell;
class CHero;
class CGTownInstance;
class DLL_EXPORT CTown
{
public:
	std::string name; //name of type
	std::vector<std::string> names; //names of the town instances
	std::vector<int> basicCreatures; //level (from 0) -> ID
	std::vector<int> upgradedCreatures; //level (from 0) -> ID
	std::map<int,int> hordeLvl; //[0] - first horde building creature level; [1] - second horde building (-1 if not present)
	ui32 mageLevel; //max available mage guild level
	int bonus; //pic number
	ui16 primaryRes;
	ui8 typeID;
};

struct DLL_EXPORT Structure
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

class DLL_EXPORT CTownHandler
{
public:
	std::vector<CTown> towns;
	std::vector<std::string> tcommands, hcommands;
	std::map<int,std::map<int, Structure*> > structures; // <town ID, <structure ID, structure>>
	std::map<int, std::map<int,std::set<int> > > requirements; //requirements[town_id][structure_id] -> set of required buildings

	CTownHandler();
	~CTownHandler();
	void loadNames();
};

#endif //CTOWNHANDLER_H
