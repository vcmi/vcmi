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
class ObjectTemplate;

struct ObjectTypeIdentifier
{
	Obj ID;
	int32_t subID;

	bool operator < (const ObjectTypeIdentifier & other) const
	{
		if (ID != other.ID)
			return ID < other.ID;
		return subID < other.subID;
	}
};

class MapIdentifiersH3M
{
	std::map<BuildingID, BuildingID> mappingBuilding;
	std::map<FactionID, std::map<BuildingID, BuildingID>> mappingFactionBuilding;
	std::map<FactionID, FactionID> mappingFaction;
	std::map<CreatureID, CreatureID> mappingCreature;
	std::map<HeroTypeID, HeroTypeID> mappingHeroType;
	std::map<HeroClassID, HeroClassID> mappingHeroClass;
	std::map<TerrainId, TerrainId> mappingTerrain;
	std::map<ArtifactID, ArtifactID> mappingArtifact;
	std::map<SecondarySkill, SecondarySkill> mappingSecondarySkill;

	std::map<std::string, std::string> mappingObjectTemplate;
	std::map<ObjectTypeIdentifier, ObjectTypeIdentifier> mappingObjectIndex;

	template<typename IdentifierID>
	void loadMapping(std::map<IdentifierID, IdentifierID> & result, const JsonNode & mapping, const std::string & identifierName);
public:
	void loadMapping(const JsonNode & mapping);

	void remapTemplate(ObjectTemplate & objectTemplate);

	BuildingID remapBuilding(std::optional<FactionID> owner, BuildingID input) const;
	FactionID remap(FactionID input) const;
	CreatureID remap(CreatureID input) const;
	HeroTypeID remap(HeroTypeID input) const;
	HeroClassID remap(HeroClassID input) const;
	TerrainId remap(TerrainId input) const;
	ArtifactID remap(ArtifactID input) const;
	SecondarySkill remap(SecondarySkill input) const;
};

VCMI_LIB_NAMESPACE_END
