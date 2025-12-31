/*
 * ObjectConfig.cpp, part of VCMI engine
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
#include "ObjectConfig.h"

#include "../GameLibrary.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

void ObjectConfig::addBannedObject(const CompoundMapObjectID & objid)
{
	// FIXME: We do not need to store the object info, just the id

	bannedObjects.push_back(objid);

	logGlobal->info("Banned object of type %d.%d", objid.primaryID, objid.secondaryID);
}

void ObjectConfig::addCustomObject(const ObjectInfo & object)
{
	customObjects.push_back(object);
	auto & lastObject = customObjects.back();
	lastObject.setAllTemplates(object.primaryID, object.secondaryID);

	// also ban object to prevent default configuration from being used in this zone
	bannedObjects.push_back(CompoundMapObjectID(object.primaryID, object.secondaryID));

	assert(lastObject.templates.size() > 0);
	logGlobal->info("Added custom object of type %d.%d", object.primaryID, object.secondaryID);
}

void ObjectConfig::serializeJson(JsonSerializeFormat & handler)
{
	static const std::map<std::string, EObjectCategory> OBJECT_CATEGORY_STRINGS = {
		{ "other", EObjectCategory::OTHER},
		{ "all", EObjectCategory::ALL},
		{ "none", EObjectCategory::NONE},
		{ "creatureBank", EObjectCategory::CREATURE_BANK},
		{ "bonus", EObjectCategory::BONUS},
		{ "dwelling", EObjectCategory::DWELLING},
		{ "resource", EObjectCategory::RESOURCE},
		{ "resourceGenerator", EObjectCategory::RESOURCE_GENERATOR},
		{ "spellScroll", EObjectCategory::SPELL_SCROLL},
		{ "randomArtifact", EObjectCategory::RANDOM_ARTIFACT},
		{ "pandorasBox", EObjectCategory::PANDORAS_BOX},
		{ "questArtifact", EObjectCategory::QUEST_ARTIFACT},
		{ "seerHut", EObjectCategory::SEER_HUT}
	};

	// Serialize banned categories
	if (handler.saving)
	{
		auto categoriesArray = handler.enterArray("bannedCategories");
		categoriesArray.syncSize(bannedObjectCategories, JsonNode::JsonType::DATA_STRING);
		
		for(size_t i = 0; i < bannedObjectCategories.size(); ++i)
		{
			for(const auto & [key, value] : OBJECT_CATEGORY_STRINGS)
			{
				if(value == bannedObjectCategories[i])
				{
					std::string categoryName = key;
					categoriesArray.serializeString(i, categoryName);
					break;
				}
			}
		}
	}
	else
	{
		const JsonNode & config = handler.getCurrent();
		const JsonNode & configBannedCategories = config["bannedCategories"];

		for(const auto & node : configBannedCategories.Vector())
		{
			auto it = OBJECT_CATEGORY_STRINGS.find(node.String());
			if(it != OBJECT_CATEGORY_STRINGS.end())
				bannedObjectCategories.push_back(it->second);
		}
	}

	// Serialize banned objects
	if (handler.saving)
	{
		// Group banned objects by primary ID
		std::map<int, std::set<int>> groupedBanned;
		for(const auto & objid : bannedObjects)
		{
			groupedBanned[objid.primaryID].insert(objid.secondaryID);
		}
		
		auto bannedObjectsStruct = handler.enterStruct("bannedObjects");
		
		for(const auto & [primaryID, secondaryIDs] : groupedBanned)
		{
			const std::string jsonKey = LIBRARY->objtypeh->getJsonKey(primaryID);
			
			if(secondaryIDs.size() == 1 && *secondaryIDs.begin() == -1)
			{
				// Ban entire object type - write as boolean true
				bool allBanned = true;
				bannedObjectsStruct->serializeBool(jsonKey, allBanned);
			}
			else
			{
				// Ban specific subtypes
				auto objStruct = bannedObjectsStruct->enterStruct(jsonKey);
				for(int secondaryID : secondaryIDs)
				{
					if(secondaryID != -1)
					{
						auto handler = LIBRARY->objtypeh->getHandlerFor(MapObjectID(primaryID), MapObjectSubID(secondaryID));
						const std::string subtypeKey = handler->getSubTypeName();
						bool banned = true;
						objStruct->serializeBool(subtypeKey, banned);
					}
				}
			}
		}
	}
	else
	{
		const JsonNode & config = handler.getCurrent();
		const JsonNode & configBannedObjects = config["bannedObjects"];

		if(configBannedObjects.isVector())
		{
			// MOD COMPATIBILITY - 1.6 format

			for(const auto & node : configBannedObjects.Vector())
			{
				LIBRARY->objtypeh->resolveObjectCompoundId(node.String(),
					[this](CompoundMapObjectID objid)
					{
						addBannedObject(objid);
					}
				);
			}
		}
		else
		{
			for(const auto & node : configBannedObjects.Struct())
			{
				LIBRARY->identifiers()->requestIdentifierIfFound(node.second.getModScope(), "object", node.first, [this, node](int primaryID)
				{
					if (node.second.isBool())
					{
						if (node.second.Bool())
							addBannedObject(CompoundMapObjectID(primaryID, -1));
					}
					else
					{
						for (const auto & subNode : node.second.Struct())
						{
							const std::string jsonKey = LIBRARY->objtypeh->getJsonKey(primaryID);

							LIBRARY->identifiers()->requestIdentifierIfFound(node.second.getModScope(), jsonKey, subNode.first, [this, primaryID](int secondaryID)
							{
								addBannedObject(CompoundMapObjectID(primaryID, secondaryID));
							});
						}
					}
				});
			}
		}
	}

	// Serialize common objects
	if (handler.saving)
	{
		auto commonObjectsArray = handler.enterArray("commonObjects");
		commonObjectsArray.syncSize(customObjects, JsonNode::JsonType::DATA_STRUCT);
		
		for(size_t i = 0; i < customObjects.size(); ++i)
		{
			auto objectStruct = commonObjectsArray.enterStruct(i);
			const auto & object = customObjects[i];
			
			// Serialize object type/subtype
			std::string objectType = LIBRARY->objtypeh->getJsonKey(object.primaryID);
			objectStruct->serializeString("type", objectType);
			
			auto handler = LIBRARY->objtypeh->getHandlerFor(MapObjectID(object.primaryID), MapObjectSubID(object.secondaryID));
			std::string subtypeName = handler->getSubTypeName();
			objectStruct->serializeString("subtype", subtypeName);
			
			// Serialize RMG properties
			auto rmgStruct = objectStruct->enterStruct("rmg");
			int value = object.value;
			int rarity = object.probability;
			int zoneLimit = (object.maxPerZone == std::numeric_limits<int>::max()) ? 0 : object.maxPerZone;
				
			rmgStruct->serializeInt("value", value);
			rmgStruct->serializeInt("rarity", rarity);
			rmgStruct->serializeInt("zoneLimit", zoneLimit);
		}
	}
	else
	{
		const JsonNode & config = handler.getCurrent();
		const JsonNode & configCommonObjects = config["commonObjects"];

		for (const auto & objectConfig : configCommonObjects.Vector())
		{
			auto rmg = objectConfig["rmg"].Struct();

			// TODO: Use common code with default rmg config

			ObjectInfo object;

			// TODO: Configure basic generateObject function
			object.value = rmg["value"].Integer();
			object.probability = rmg["rarity"].Integer();
			object.maxPerZone = rmg["zoneLimit"].Integer();
			if (object.maxPerZone == 0)
				object.maxPerZone = std::numeric_limits<int>::max();

			if (objectConfig["id"].isNull())
			{
				LIBRARY->identifiers()->requestIdentifierIfFound("object", objectConfig["type"], [this, object, objectConfig](int primaryID)
				{
					if (objectConfig["subtype"].isNull())
					{
						auto objectWithID = object;
						objectWithID.primaryID = primaryID;
						objectWithID.secondaryID = 0;
						addCustomObject(objectWithID);
					}
					else
					{
						const std::string jsonKey = LIBRARY->objtypeh->getJsonKey(primaryID);

						LIBRARY->identifiers()->requestIdentifierIfFound(jsonKey, objectConfig["subtype"], [this, primaryID, object](int secondaryID)
						{
							auto objectWithID = object;
							objectWithID.primaryID = primaryID;
							objectWithID.secondaryID = secondaryID;
							addCustomObject(objectWithID);
						});
					}
				});
			}
			else
			{
				// MOD COMPATIBILITY - 1.6 format
				auto objectName = objectConfig["id"].String();

				LIBRARY->objtypeh->resolveObjectCompoundId(objectName, [this, object](CompoundMapObjectID objid)
				{
					auto objectWithID = object;
					objectWithID.primaryID = objid.primaryID;
					objectWithID.secondaryID = objid.secondaryID;
					if (objectWithID.secondaryID == -1)
						objectWithID.secondaryID = 0;
					addCustomObject(objectWithID);
				});
			}
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
