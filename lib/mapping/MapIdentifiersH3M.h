/*
 * MapIdentifiersH3M.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

class MapIdentifiersH3M
{
	std::map<BuildingID, BuildingID> mappingBuilding;
	std::map<FactionID, std::map<BuildingID, BuildingID>> mappingFactionBuilding;
	//std::map<FactionID, FactionID> mappingFaction;
	//std::map<CreatureID, CreatureID> mappingCreature;
	//std::map<HeroTypeID, HeroTypeID> mappingHeroType;
	//std::map<HeroClassID, HeroClassID> mappingHeroClass;
	//std::map<TerrainId, TerrainId> mappingTerrain;
	//std::map<ArtifactID, ArtifactID> mappingArtifact;
	//std::map<SecondarySkill, SecondarySkill> mappingSecondarySkill;
public:
	void loadMapping(const JsonNode & mapping);

	BuildingID remapBuilding(FactionID owner, BuildingID input) const;
	BuildingID remapBuilding(BuildingID input) const;
	//FactionID remapFaction(FactionID input) const;
	//CreatureID remapCreature(CreatureID input) const;
	//HeroTypeID remapHeroType(HeroTypeID input) const;
	//HeroClassID remapHeroClass(HeroClassID input) const;
	//TerrainId remapTerrain(TerrainId input) const;
	//ArtifactID remapArtifact(ArtifactID input) const;
	//SecondarySkill remapSecondarySkill(SecondarySkill input) const;
};

VCMI_LIB_NAMESPACE_END
