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
#include "../filesystem/ResourcePath.h"
#include "../json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

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

class DLL_LINKAGE MapIdentifiersH3M
{
	std::map<BuildingID, BuildingID> mappingBuilding;
	std::map<FactionID, std::map<BuildingID, BuildingID>> mappingFactionBuilding;
	std::map<FactionID, FactionID> mappingFaction;
	std::map<CreatureID, CreatureID> mappingCreature;
	std::map<HeroTypeID, HeroTypeID> mappingHeroType;
	std::map<HeroTypeID, HeroTypeID> mappingHeroPortrait;
	std::map<HeroClassID, HeroClassID> mappingHeroClass;
	std::map<TerrainId, TerrainId> mappingTerrain;
	std::map<ArtifactID, ArtifactID> mappingArtifact;
	std::map<SecondarySkill, SecondarySkill> mappingSecondarySkill;
	std::map<CampaignRegionID, CampaignRegionID> mappingCampaignRegions;
	std::map<int, std::pair<VideoPath, VideoPath>> mappingCampaignVideo;
	std::map<int, AudioPath> mappingCampaignMusic;

	std::map<AnimationPath, AnimationPath> mappingObjectTemplate;
	std::map<ObjectTypeIdentifier, ObjectTypeIdentifier> mappingObjectIndex;

	JsonNode formatSettings;

	template<typename IdentifierID>
	void loadMapping(std::map<IdentifierID, IdentifierID> & result, const JsonNode & mapping, const std::string & identifierName);
public:
	void loadMapping(const JsonNode & mapping);

	void remapTemplate(ObjectTemplate & objectTemplate);

	AudioPath remapCampaignMusic(int index) const;
	std::pair<VideoPath, VideoPath> remapCampaignVideo(int index) const;
	BuildingID remapBuilding(std::optional<FactionID> owner, BuildingID input) const;
	HeroTypeID remapPortrait(HeroTypeID input) const;
	FactionID remap(FactionID input) const;
	CreatureID remap(CreatureID input) const;
	HeroTypeID remap(HeroTypeID input) const;
	HeroClassID remap(HeroClassID input) const;
	TerrainId remap(TerrainId input) const;
	ArtifactID remap(ArtifactID input) const;
	SecondarySkill remap(SecondarySkill input) const;
	CampaignRegionID remap(CampaignRegionID input) const;

	const JsonNode & getFormatSettings() const { return formatSettings; }

};

VCMI_LIB_NAMESPACE_END
