/*
 * ObjectInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include <boost/bimap.hpp>
#include <boost/assign.hpp>
#include "ObjectInfo.h"

#include "../VCMI_Lib.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

ObjectInfo::ObjectInfo():
	destroyObject([](CGObjectInstance * obj){}),
	maxPerZone(std::numeric_limits<ui32>::max())
{
}

ObjectInfo::ObjectInfo(const ObjectInfo & other)
{
	templates = other.templates;
	value = other.value;
	probability = other.probability;
	maxPerZone = other.maxPerZone;
	generateObject = other.generateObject;
	destroyObject = other.destroyObject;
}

ObjectInfo & ObjectInfo::operator=(const ObjectInfo & other)
{
	if (this == &other)
		return *this;

	templates = other.templates;
	value = other.value;
	probability = other.probability;
	maxPerZone = other.maxPerZone;
	generateObject = other.generateObject;
	destroyObject = other.destroyObject;
	return *this;
}

void ObjectInfo::setTemplates(MapObjectID type, MapObjectSubID subtype, TerrainId terrainType)
{
	auto templHandler = VLC->objtypeh->getHandlerFor(type, subtype);
	if(!templHandler)
		return;
	
	templates = templHandler->getTemplates(terrainType);
}

void ObjectConfig::addBannedObject(const CompoundMapObjectID & objid)
{
	// FIXME: We do not need to store the object info, just the id

	bannedObjects.push_back(objid);

	logGlobal->info("Banned object of type %d.%d", objid.primaryID, objid.secondaryID);
}

void ObjectConfig::serializeJson(JsonSerializeFormat & handler)
{
	// TODO: We need serializer utility for list of enum values

	static const boost::bimap<EObjectCategory, std::string> OBJECT_CATEGORY_STRINGS = boost::assign::list_of<boost::bimap<EObjectCategory, std::string>::relation>
		(EObjectCategory::OTHER, "other")
		(EObjectCategory::ALL, "all")
		(EObjectCategory::NONE, "none")
		(EObjectCategory::CREATURE_BANK, "creatureBank")
		(EObjectCategory::BONUS, "bonus")
		(EObjectCategory::DWELLING, "dwelling")
		(EObjectCategory::RESOURCE, "resource")
		(EObjectCategory::RESOURCE_GENERATOR, "resourceGenerator")
		(EObjectCategory::SPELL_SCROLL, "spellScroll")
		(EObjectCategory::RANDOM_ARTIFACT, "randomArtifact")
		(EObjectCategory::PANDORAS_BOX, "pandorasBox")
		(EObjectCategory::QUEST_ARTIFACT, "questArtifact")
		(EObjectCategory::SEER_HUT, "seerHut");

	auto categories = handler.enterArray("bannedCategories");
	if (handler.saving)
	{
		for (const auto& category : bannedObjectCategories)
		{
			auto str = OBJECT_CATEGORY_STRINGS.left.at(category);
			categories.serializeString(categories.size(), str);
		}
	}
	else
	{
		std::vector<std::string> categoryNames;
		categories.serializeArray(categoryNames);

		for (const auto & categoryName : categoryNames)
		{
			auto it = OBJECT_CATEGORY_STRINGS.right.find(categoryName);
			if (it != OBJECT_CATEGORY_STRINGS.right.end())
			{
				bannedObjectCategories.push_back(it->second);
			}
		}
	}
	
	auto bannedObjectData = handler.enterArray("bannedObjects");	
	if (handler.saving)
	{

		// FIXME: Do we even need to serialize / store banned objects?
		/*
		for (const auto & object : bannedObjects)
		{
			// TODO: Translate id back to string?


			JsonNode node;
			node.String() = VLC->objtypeh->getHandlerFor(object.primaryID, object.secondaryID);
			// TODO: Check if AI-generated code is right


		}
		// handler.serializeRaw("bannedObjects", node, std::nullopt);

		*/
	}
	else
	{
		/*
			auto zonesData = handler.enterStruct("zones");
			for(const auto & idAndZone : zonesData->getCurrent().Struct())
			{
				auto guard = handler.enterStruct(idAndZone.first);
				auto zone = std::make_shared<ZoneOptions>();
				zone->setId(decodeZoneId(idAndZone.first));
				zone->serializeJson(handler);
				zones[zone->getId()] = zone;
			}
		*/
		std::vector<std::string> objectNames;
		bannedObjectData.serializeArray(objectNames);

		for (const auto & objectName : objectNames)
		{
			VLC->objtypeh->resolveObjectCompoundId(objectName,
				[this](CompoundMapObjectID objid)
				{
					addBannedObject(objid);
				}
			);
			
		}
	}
}

const std::vector<ObjectInfo> & ObjectConfig::getConfiguredObjects() const
{
	return customObjects;
}

const std::vector<CompoundMapObjectID> & ObjectConfig::getBannedObjects() const
{
	return bannedObjects;
}

const std::vector<ObjectConfig::EObjectCategory> & ObjectConfig::getBannedObjectCategories() const
{
	return bannedObjectCategories;
}

VCMI_LIB_NAMESPACE_END