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

template<typename IdentifierID>
std::map<IdentifierID, IdentifierID> MapIdentifiersH3M::loadMapping(const JsonNode & mapping, const std::string & identifierName)
{
	std::map<IdentifierID, IdentifierID> result;

	for (auto entry : mapping.Struct())
	{
		IdentifierID sourceID (entry.second.Integer());
		IdentifierID targetID (*VLC->modh->identifiers.getIdentifier(VLC->modh->scopeGame(), identifierName, entry.first));

		result[sourceID] = targetID;
	}

	return result;
}

void MapIdentifiersH3M::loadMapping(const JsonNode & mapping)
{
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

	mappingBuilding = loadMapping<BuildingID>(mapping["buildingsCommon"], "building.core:random");
	mappingFaction = loadMapping<FactionID>(mapping["factions"], "faction");
	mappingCreature = loadMapping<CreatureID>(mapping["creatures"], "creature");
	mappingHeroType = loadMapping<HeroTypeID>(mapping["heroes"], "hero");
	mappingHeroClass = loadMapping<HeroClassID>(mapping["heroClasses"], "heroClass");
	mappingTerrain = loadMapping<TerrainId>(mapping["terrains"], "terrain");
	mappingArtifact = loadMapping<ArtifactID>(mapping["artifacts"], "artifact");
	mappingSecondarySkill = loadMapping<SecondarySkill>(mapping["skills"], "skill");
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

FactionID MapIdentifiersH3M::remap(FactionID input) const
{
	if (mappingFaction.count(input))
		return mappingFaction.at(input);
	return input;
}

CreatureID MapIdentifiersH3M::remap(CreatureID input) const
{
	if (mappingCreature.count(input))
		return mappingCreature.at(input);
	return input;
}

HeroTypeID MapIdentifiersH3M::remap(HeroTypeID input) const
{
	if (mappingHeroType.count(input))
		return mappingHeroType.at(input);
	return input;
}

HeroClassID MapIdentifiersH3M::remap(HeroClassID input) const
{
	if (mappingHeroClass.count(input))
		return mappingHeroClass.at(input);
	return input;
}

TerrainId MapIdentifiersH3M::remap(TerrainId input) const
{
	if (mappingTerrain.count(input))
		return mappingTerrain.at(input);
	return input;
}

ArtifactID MapIdentifiersH3M::remap(ArtifactID input) const
{
	if (mappingArtifact.count(input))
		return mappingArtifact.at(input);
	return input;
}

SecondarySkill MapIdentifiersH3M::remap(SecondarySkill input) const
{
	if (mappingSecondarySkill.count(input))
		return mappingSecondarySkill.at(input);
	return input;
}

VCMI_LIB_NAMESPACE_END
