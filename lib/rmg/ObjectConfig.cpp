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

void ObjectConfig::addRequiredObject(const CompoundMapObjectID & objid, ui16 count, std::optional<ui32> guardLevel)
{
	requiredObjects[objid] = std::pair{count, guardLevel};

	logGlobal->info("Added required object of type %d.%d, count: %d, guard level: %d",
		objid.primaryID, objid.secondaryID, count,
		guardLevel.has_value() ? std::to_string(guardLevel.value()) : "none");
}

void ObjectConfig::addRequiredObject(const CompoundMapObjectID & objid, std::pair<ui16, std::optional<ui32>> info)
{
	addRequiredObject(objid, info.first, info.second);
}

void ObjectConfig::clearRequiredObjects()
{
	requiredObjects.clear();
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

	// Serialize required objects
	if(handler.saving)
	{
		if(!requiredObjects.empty())
		{
			auto requiredObjectsStruct = handler.enterStruct("requiredObjects");
			for(const auto & [objId, info] : requiredObjects)
			{
				std::string jsonKey = LIBRARY->objtypeh->getJsonKey(objId.primaryID);
				auto entryStruct = requiredObjectsStruct->enterStruct(jsonKey);
				auto objHandler = LIBRARY->objtypeh->getHandlerFor(objId);
				std::string jsonSubKey = objHandler->getJsonKey();
				auto subEntryStruct = requiredObjectsStruct->enterStruct(jsonSubKey);

				ui16 countVal = info.first;
				subEntryStruct->serializeInt("count", countVal);
				if (info.second.has_value())
				{
					ui32 guardVal = info.second.value();
					subEntryStruct->serializeInt("guard", guardVal);
				}
			}
		}
	}
	else
	{
		const JsonNode & config = handler.getCurrent();
		const JsonNode & configRequiredObjects = config["requiredObjects"];

		const auto getInfo = [](const JsonNode & node) {
			auto & child = node.Struct();
			ui16 countVal = static_cast<ui16>(child.at("count").Integer());
			std::optional<ui32> guardVal = std::nullopt;
			if (child.contains("guard"))
				guardVal = static_cast<ui32>(child.at("guard").Integer());
			return std::pair{countVal, guardVal};
		};

		for(const auto & node : configRequiredObjects.Struct())
		{
			LIBRARY->identifiers()->requestIdentifierIfFound(node.second.getModScope(), "object", node.first, [this, node, &getInfo](int primaryID)
			{
				if (node.second.isNumber())
				{
					addRequiredObject(CompoundMapObjectID(primaryID, -1), static_cast<ui16>(node.second.Integer()));
				}
				else
				{
					if (node.second.Struct().contains("count"))
					{
						addRequiredObject(CompoundMapObjectID(primaryID, -1), getInfo(node.second));
					}
					else
					{
						// secondary ID specified
						for (const auto & subNode : node.second.Struct())
						{
							std::string jsonKey = LIBRARY->objtypeh->getJsonKey(primaryID);
							if(node.first == "artifact" || node.first == "core:artifact")
								//FIXME: core:artifact is not resolved properly by requestIdentifierIfFound
								jsonKey = "artifact";

							LIBRARY->identifiers()->requestIdentifierIfFound(node.second.getModScope(), jsonKey, subNode.first, [this, primaryID, subNode, &getInfo](int secondaryID)
							{
								if(subNode.second.isNumber())
									addRequiredObject(CompoundMapObjectID(primaryID, secondaryID), static_cast<ui16>(subNode.second.Integer()));
								else
									addRequiredObject(CompoundMapObjectID(primaryID, secondaryID), getInfo(subNode.second));
							});
						}
					}
				}
			});
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

std::map<CompoundMapObjectID, std::pair<ui16, std::optional<ui32>>> ObjectConfig::getRequiredObjects() const
{
	return requiredObjects;
}

VCMI_LIB_NAMESPACE_END
