/*
 * CBuildingHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBuildingHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

BuildingID CBuildingHandler::campToERMU( int camp, int townType, std::set<BuildingID> builtBuildings )
{
	static const std::vector<BuildingID> campToERMU = 
	{
		BuildingID::TOWN_HALL, BuildingID::CITY_HALL,
		BuildingID::CAPITOL, BuildingID::FORT, BuildingID::CITADEL, BuildingID::CASTLE, BuildingID::TAVERN,
		BuildingID::BLACKSMITH, BuildingID::MARKETPLACE, BuildingID::RESOURCE_SILO, BuildingID::NONE,
		BuildingID::MAGES_GUILD_1, BuildingID::MAGES_GUILD_2, BuildingID::MAGES_GUILD_3, BuildingID::MAGES_GUILD_4,
		BuildingID::MAGES_GUILD_5,
		BuildingID::SHIPYARD, BuildingID::GRAIL,
		BuildingID::SPECIAL_1, BuildingID::SPECIAL_2, BuildingID::SPECIAL_3, BuildingID::SPECIAL_4	
	}; //creature generators with banks - handled separately
	
	if (camp < campToERMU.size())
	{
		return campToERMU[camp];
	}

	static const std::vector<int> hordeLvlsPerTType[GameConstants::F_NUMBER] = 
	{
		{2}, {1}, {1,4}, {0,2},	{0}, {0}, {0}, {0}, {0}
	};

	int curPos = static_cast<int>(campToERMU.size());
	for (int i=0; i<GameConstants::CREATURES_PER_TOWN; ++i)
	{
		if(camp == curPos) //non-upgraded
			return BuildingID(30 + i);
		curPos++;
		if(camp == curPos) //upgraded
			return BuildingID(37 + i);
		curPos++;

		if (i < 5) // last two levels don't have reserved horde ID. Yet another H3C weirdeness
		{
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
			}
			curPos++;
		}
	}
	assert(0);
	return BuildingID::NONE; //not found
}


VCMI_LIB_NAMESPACE_END
