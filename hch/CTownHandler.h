#ifndef __CTOWNHANDLER_H__
#define __CTOWNHANDLER_H__
#include "../global.h"
#include <set>

/*
 * CTownHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CBuilding;
class CSpell;
class CHero;
class CGTownInstance;
class DLL_EXPORT CTown
{
	std::string name; //name of type
public:

	std::vector<std::string> names; //names of the town instances
	std::vector<int> basicCreatures; //level (from 0) -> ID
	std::vector<int> upgradedCreatures; //level (from 0) -> ID
	std::map<int,int> hordeLvl; //[0] - first horde building creature level; [1] - second horde building (-1 if not present)
	ui32 mageLevel; //max available mage guild level
	int bonus; //pic number
	ui16 primaryRes, warMachine;
	ui8 typeID;


	const std::vector<std::string> & Names() const;
	const std::string & Name() const;
	void Name(const std::string & val) { name = val; }


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & names & basicCreatures & upgradedCreatures & hordeLvl & mageLevel & bonus 
			& primaryRes & warMachine & typeID;
	}
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
	std::vector<std::map<int, Structure*> > structures; // <town ID, <structure ID, structure>>
	std::vector<std::map<int,std::set<int> > > requirements; //requirements[town_id][structure_id] -> set of required buildings

	CTownHandler(); //c-tor
	~CTownHandler(); //d-tor
	void loadNames();
	void loadStructures();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & towns & requirements;
		if(!h.saving)
			loadStructures();
	}
};


#endif // __CTOWNHANDLER_H__
