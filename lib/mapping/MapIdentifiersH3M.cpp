/*
 * MapIdentifiersH3M.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapIdentifiersH3M.h"

#include "../JsonNode.h"
#include "../VCMI_Lib.h"
#include "../CModHandler.h"
#include "../CTownHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

void MapIdentifiersH3M::loadMapping(const JsonNode & mapping)
{
	for (auto entry : mapping["buildingsCommon"].Struct())
	{
		BuildingID sourceID (entry.second.Integer());
		BuildingID targetID (*VLC->modh->identifiers.getIdentifier(VLC->modh->scopeGame(), "building.core:random", entry.first));

		mappingBuilding[sourceID] = targetID;
	}

	for (auto entryFaction : mapping["buildings"].Struct())
	{
		FactionID factionID (*VLC->modh->identifiers.getIdentifier(VLC->modh->scopeGame(), "faction", entryFaction.first));
		auto buildingMap = entryFaction.second;

		for (auto entryBuilding : buildingMap.Struct())
		{
			BuildingID sourceID (entryBuilding.second.Integer());
			BuildingID targetID (*VLC->modh->identifiers.getIdentifier(VLC->modh->scopeGame(), "building." + VLC->factions()->getById(factionID)->getJsonKey(), entryBuilding.first));

			mappingFactionBuilding[factionID][sourceID] = targetID;
		}
	}
}

BuildingID MapIdentifiersH3M::remapBuilding(std::optional<FactionID> owner, BuildingID input) const
{
	if (owner.has_value() && mappingFactionBuilding.count(*owner))
	{
		auto submap = mappingFactionBuilding.at(*owner);

		if (submap.count(input))
			return submap.at(input);
	}

	if (mappingBuilding.count(input))
		return mappingBuilding.at(input);
	return BuildingID::NONE;
}

VCMI_LIB_NAMESPACE_END
