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
#include "../CTownHandler.h"
#include "../CHeroHandler.h"
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
		IdentifierID targetID (*VLC->identifiers()->getIdentifier(entry.second.meta, identifierName, entry.first));

		result[sourceID] = targetID;
	}
}

void MapIdentifiersH3M::loadMapping(const JsonNode & mapping)
{
	for (auto entryFaction : mapping["buildings"].Struct())
	{
		FactionID factionID (*VLC->identifiers()->getIdentifier(entryFaction.second.meta, "faction", entryFaction.first));
		auto buildingMap = entryFaction.second;

		for (auto entryBuilding : buildingMap.Struct())
		{
			BuildingID sourceID (entryBuilding.second.Integer());
			BuildingID targetID (*VLC->identifiers()->getIdentifier(entryBuilding.second.meta, "building." + VLC->factions()->getById(factionID)->getJsonKey(), entryBuilding.first));

			mappingFactionBuilding[factionID][sourceID] = targetID;
		}
	}

	for (auto entryTemplate : mapping["templates"].Struct())
	{
		std::string h3mName = boost::to_lower_copy(entryTemplate.second.String());
		std::string vcmiName = boost::to_lower_copy(entryTemplate.first);

		if (!CResourceHandler::get()->existsResource(ResourcePath( "SPRITES/" + vcmiName, EResType::ANIMATION)))
			logMod->warn("Template animation file %s was not found!", vcmiName);

		mappingObjectTemplate[h3mName] = vcmiName;
	}

	for (auto entryOuter : mapping["objects"].Struct())
	{
		if (entryOuter.second.isStruct())
		{
			for (auto entryInner : entryOuter.second.Struct())
			{
				auto handler = VLC->objtypeh->getHandlerFor( entryInner.second.meta, entryOuter.first, entryInner.first);

				auto entryValues = entryInner.second.Vector();
				ObjectTypeIdentifier h3mID{Obj(entryValues[0].Integer()), int32_t(entryValues[1].Integer())};
				ObjectTypeIdentifier vcmiID{Obj(handler->getIndex()), handler->getSubIndex()};
				mappingObjectIndex[h3mID] = vcmiID;
			}
		}
		else
		{
			auto handler = VLC->objtypeh->getHandlerFor( entryOuter.second.meta, entryOuter.first, entryOuter.first);

			auto entryValues = entryOuter.second.Vector();
			ObjectTypeIdentifier h3mID{Obj(entryValues[0].Integer()), int32_t(entryValues[1].Integer())};
			ObjectTypeIdentifier vcmiID{Obj(handler->getIndex()), handler->getSubIndex()};
			mappingObjectIndex[h3mID] = vcmiID;
		}
	}

	for (auto entry : mapping["portraits"].Struct())
	{
		int32_t sourceID = entry.second.Integer();
		int32_t targetID = *VLC->identifiers()->getIdentifier(entry.second.meta, "hero", entry.first);
		int32_t iconID = VLC->heroTypes()->getByIndex(targetID)->getIconIndex();

		mappingHeroPortrait[sourceID] = iconID;
	}

	loadMapping(mappingBuilding, mapping["buildingsCommon"], "building.core:random");
	loadMapping(mappingFaction, mapping["factions"], "faction");
	loadMapping(mappingCreature, mapping["creatures"], "creature");
	loadMapping(mappingHeroType, mapping["heroes"], "hero");
	loadMapping(mappingHeroClass, mapping["heroClasses"], "heroClass");
	loadMapping(mappingTerrain, mapping["terrains"], "terrain");
	loadMapping(mappingArtifact, mapping["artifacts"], "artifact");
	loadMapping(mappingSecondarySkill, mapping["skills"], "skill");
}

void MapIdentifiersH3M::remapTemplate(ObjectTemplate & objectTemplate)
{
	std::string name = boost::to_lower_copy(objectTemplate.animationFile.getName());

	if (mappingObjectTemplate.count(name))
		objectTemplate.animationFile = AnimationPath::builtinTODO(mappingObjectTemplate.at(name));

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

int32_t MapIdentifiersH3M::remapPortrrait(int32_t input) const
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

VCMI_LIB_NAMESPACE_END
