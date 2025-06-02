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

#include "../GameLibrary.h"
#include "../entities/faction/CFaction.h"
#include "../entities/faction/CTownHandler.h"
#include "../filesystem/Filesystem.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../modding/IdentifierStorage.h"

VCMI_LIB_NAMESPACE_BEGIN

template<typename IdentifierID>
void MapIdentifiersH3M::loadMapping(std::map<IdentifierID, IdentifierID> & result, const JsonNode & mapping, const std::string & identifierName)
{
	for (auto entry : mapping.Struct())
	{
		IdentifierID sourceID (entry.second.Integer());
		IdentifierID targetID (*LIBRARY->identifiers()->getIdentifier(entry.second.getModScope(), identifierName, entry.first));

		result[sourceID] = targetID;
	}
}

void MapIdentifiersH3M::loadMapping(const JsonNode & mapping)
{
	if (!mapping["supported"].Bool())
		throw std::runtime_error("Unsupported map format!");

	for (auto entryFaction : mapping["buildings"].Struct())
	{
		FactionID factionID (*LIBRARY->identifiers()->getIdentifier(entryFaction.second.getModScope(), "faction", entryFaction.first));
		auto buildingMap = entryFaction.second;

		for (auto entryBuilding : buildingMap.Struct())
		{
			BuildingID sourceID (entryBuilding.second.Integer());
			BuildingID targetID (*LIBRARY->identifiers()->getIdentifier(entryBuilding.second.getModScope(), "building." + LIBRARY->factions()->getById(factionID)->getJsonKey(), entryBuilding.first));

			mappingFactionBuilding[factionID][sourceID] = targetID;
		}
	}

	for (auto entryTemplate : mapping["templates"].Struct())
	{
		AnimationPath h3mName = AnimationPath::builtinTODO(entryTemplate.second.String());
		AnimationPath vcmiName = AnimationPath::builtinTODO(entryTemplate.first);

		if (!CResourceHandler::get()->existsResource(vcmiName.addPrefix("SPRITES/")))
			logMod->warn("Template animation file %s was not found!", vcmiName.getOriginalName());

		mappingObjectTemplate[h3mName] = vcmiName;
	}

	for (auto entryOuter : mapping["objects"].Struct())
	{
		if (entryOuter.second.isStruct())
		{
			for (auto entryInner : entryOuter.second.Struct())
			{
				auto handler = LIBRARY->objtypeh->getHandlerFor( entryInner.second.getModScope(), entryOuter.first, entryInner.first);

				auto entryValues = entryInner.second.Vector();
				ObjectTypeIdentifier h3mID{Obj(entryValues[0].Integer()), int32_t(entryValues[1].Integer())};
				ObjectTypeIdentifier vcmiID{Obj(handler->getIndex()), handler->getSubIndex()};
				mappingObjectIndex[h3mID] = vcmiID;
			}
		}
		else
		{
			auto handler = LIBRARY->objtypeh->getHandlerFor( entryOuter.second.getModScope(), entryOuter.first, entryOuter.first);

			auto entryValues = entryOuter.second.Vector();
			ObjectTypeIdentifier h3mID{Obj(entryValues[0].Integer()), int32_t(entryValues[1].Integer())};
			ObjectTypeIdentifier vcmiID{Obj(handler->getIndex()), handler->getSubIndex()};
			mappingObjectIndex[h3mID] = vcmiID;
		}
	}

	for (auto entry : mapping["campaignVideo"].Struct())
		mappingCampaignVideo[entry.second.Integer()] = VideoPath::builtinTODO(entry.first);

	for (auto entry : mapping["campaignMusic"].Struct())
		mappingCampaignMusic[entry.second.Integer()] = AudioPath::builtinTODO(entry.first);

	loadMapping(mappingHeroPortrait, mapping["portraits"], "hero");
	loadMapping(mappingBuilding, mapping["buildingsCommon"], "building.core:random");
	loadMapping(mappingFaction, mapping["factions"], "faction");
	loadMapping(mappingCreature, mapping["creatures"], "creature");
	loadMapping(mappingHeroType, mapping["heroes"], "hero");
	loadMapping(mappingHeroClass, mapping["heroClasses"], "heroClass");
	loadMapping(mappingTerrain, mapping["terrains"], "terrain");
	loadMapping(mappingArtifact, mapping["artifacts"], "artifact");
	loadMapping(mappingSecondarySkill, mapping["skills"], "skill");
	loadMapping(mappingCampaignRegions, mapping["campaignRegions"], "campaignRegion");
}

void MapIdentifiersH3M::remapTemplate(ObjectTemplate & objectTemplate)
{
	auto name = objectTemplate.animationFile;

	if (mappingObjectTemplate.count(name))
		objectTemplate.animationFile = mappingObjectTemplate.at(name);

	ObjectTypeIdentifier objectType{ objectTemplate.id, objectTemplate.subid};

	if (mappingObjectIndex.count(objectType))
	{
		auto mappedType = mappingObjectIndex.at(objectType);
		objectTemplate.id = mappedType.ID;
		objectTemplate.subid = mappedType.subID;
	}

	if (objectTemplate.id == Obj::TOWN || objectTemplate.id == Obj::RANDOM_DWELLING_FACTION)
		objectTemplate.subid = remap(FactionID(objectTemplate.subid));

	if (objectTemplate.id == Obj::MONSTER)
		objectTemplate.subid = remap(CreatureID(objectTemplate.subid));

	if (objectTemplate.id == Obj::ARTIFACT)
		objectTemplate.subid = remap(ArtifactID(objectTemplate.subid));

	if (LIBRARY->objtypeh->knownObjects().count(objectTemplate.id) == 0)
	{
		logGlobal->warn("Unknown object found: %d | %d", objectTemplate.id, objectTemplate.subid);

		objectTemplate.id = Obj::NOTHING;
		objectTemplate.subid = {};
	}
	else
	{
		if (LIBRARY->objtypeh->knownSubObjects(objectTemplate.id).count(objectTemplate.subid) == 0)
		{
			logGlobal->warn("Unknown subobject found: %d | %d", objectTemplate.id, objectTemplate.subid);
			objectTemplate.subid = {};
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

HeroTypeID MapIdentifiersH3M::remapPortrait(HeroTypeID input) const
{
	if (mappingHeroPortrait.count(input))
		return mappingHeroPortrait.at(input);
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

CampaignRegionID MapIdentifiersH3M::remap(CampaignRegionID input) const
{
	if (!mappingCampaignRegions.count(input))
		throw std::out_of_range("Campaign region with ID " + std::to_string(input.getNum()) + " is not defined");

	return mappingCampaignRegions.at(input);
}

VideoPath MapIdentifiersH3M::remapCampaignVideo(int input) const
{
	if (!mappingCampaignVideo.count(input))
		throw std::out_of_range("Campaign video with ID " + std::to_string(input) + " is not defined");

	return mappingCampaignVideo.at(input);
}

AudioPath MapIdentifiersH3M::remapCampaignMusic(int input) const
{
	if (!mappingCampaignMusic.count(input))
		throw std::out_of_range("Campaign music with ID " + std::to_string(input) + " is not defined");

	return mappingCampaignMusic.at(input);
}

VCMI_LIB_NAMESPACE_END
