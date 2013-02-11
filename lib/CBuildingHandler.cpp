#include "StdInc.h"
#include "CBuildingHandler.h"


/*
 * CBuildingHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

BuildingID CBuildingHandler::campToERMU( int camp, int townType, std::set<BuildingID> builtBuildings )
{
	using namespace boost::assign;
	static const std::vector<int> campToERMU = list_of(11)(12)(13)(7)(8)(9)(5)(16)(14)(15)(-1)(0)(1)(2)(3)(4)
		(6)(26)(17)(21)(22)(23)
		; //creature generators with banks - handled separately
	if (camp < campToERMU.size())
	{
		return BuildingID(campToERMU[camp]);
	}

	static const std::vector<int> hordeLvlsPerTType[GameConstants::F_NUMBER] = {list_of(2), list_of(1), list_of(1)(4), list_of(0)(2),
		list_of(0), list_of(0), list_of(0), list_of(0), list_of(0)};

	int curPos = campToERMU.size();
	for (int i=0; i<7; ++i)
	{
		if(camp == curPos) //non-upgraded
			return BuildingID(30 + i);
		curPos++;
		if(camp == curPos) //upgraded
			return BuildingID(37 + i);
		curPos++;
		//horde building
		if (vstd::contains(hordeLvlsPerTType[townType], i))
		{
			if (camp == curPos)
			{
				if (hordeLvlsPerTType[townType][0] == i)
				{
					if(vstd::contains(builtBuildings, 37 + hordeLvlsPerTType[townType][0])) //if upgraded dwelling is built
						return BuildingID::HORDE_1_UPGR;
					else //upgraded dwelling not presents
						return BuildingID::HORDE_1;
				}
				else
				{
					if(hordeLvlsPerTType[townType].size() > 1)
					{
						if(vstd::contains(builtBuildings, 37 + hordeLvlsPerTType[townType][1])) //if upgraded dwelling is built
							return BuildingID::HORDE_2_UPGR;
						else //upgraded dwelling not presents
							return BuildingID::HORDE_2;
					}
				}
			}
			curPos++;
		}

	}
	assert(0);
	return BuildingID::NONE; //not found
}

