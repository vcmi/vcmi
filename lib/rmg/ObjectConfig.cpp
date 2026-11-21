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
#include "ObjectInfo.h"
#include "ObjectConfig.h"

#include "../GameLibrary.h"
#include "../modding/IdentifierResolver.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/IObjectInfo.h"
#include "../serializer/JsonSerializeFormat.h"

namespace
{
ObjectConfig::EObjectCategory getObjectCategory(CompoundMapObjectID id)
{
	auto name = LIBRARY->objtypeh->getObjectHandlerName(id.primaryID);

	if (name == "configurable")
	{
		auto handler = LIBRARY->objtypeh->getHandlerFor(id.primaryID, id.secondaryID);
		if (!handler)
		{
			return ObjectConfig::EObjectCategory::NONE;
		}

		auto info = handler->getObjectInfo();

		if (info->hasGuards())
		{
			return ObjectConfig::EObjectCategory::CREATURE_BANK;
		}
		else if (info->givesResources())
		{
			return ObjectConfig::EObjectCategory::RESOURCE;
		}
		else if (info->givesArtifacts())
		{
			return ObjectConfig::EObjectCategory::RANDOM_ARTIFACT;
		}
		else if (info->givesBonuses())
		{
			return ObjectConfig::EObjectCategory::BONUS;
		}

		return ObjectConfig::EObjectCategory::OTHER;
	}
	else if (name == "dwelling" || name == "randomDwelling")
	{
		return ObjectConfig::EObjectCategory::DWELLING;
	}
	else if (name == "bank")
		return ObjectConfig::EObjectCategory::CREATURE_BANK;
	else if (name == "market")
		return ObjectConfig::EObjectCategory::OTHER;
	else if (name == "hillFort")
		return ObjectConfig::EObjectCategory::OTHER;
	else if (name == "resource" || name == "randomResource")
		return ObjectConfig::EObjectCategory::RESOURCE;
	else if (name == "randomArtifact")
		return ObjectConfig::EObjectCategory::RANDOM_ARTIFACT;
	else if (name == "artifact")
	{
		if (id.primaryID == Obj::SPELL_SCROLL)
		{
			return ObjectConfig::EObjectCategory::SPELL_SCROLL;
		}
		else
		{
			return ObjectConfig::EObjectCategory::QUEST_ARTIFACT;
		}
	}
	else if (name == "denOfThieves")
		return ObjectConfig::EObjectCategory::OTHER;
	else if (name == "lighthouse")
	{
		return ObjectConfig::EObjectCategory::BONUS;
	}
	else if (name == "magi")
	{
		return ObjectConfig::EObjectCategory::OTHER;
	}
	else if (name == "mine")
		return ObjectConfig::EObjectCategory::RESOURCE_GENERATOR;
	else if (name == "pandora")
		return ObjectConfig::EObjectCategory::PANDORAS_BOX;
	else if (name == "prison")
	{
		return ObjectConfig::EObjectCategory::OTHER;
	}
	else if (name == "questArtifact")
	{
		return ObjectConfig::EObjectCategory::QUEST_ARTIFACT;
	}
	else if (name == "seerHut")
	{
		return ObjectConfig::EObjectCategory::SEER_HUT;
	}
	else if (name == "siren")
		return ObjectConfig::EObjectCategory::BONUS;
	else if (name == "obelisk")
		return ObjectConfig::EObjectCategory::OTHER;

	return ObjectConfig::EObjectCategory::NONE;
}

bool categoryContains(const std::vector<ObjectConfig::EObjectCategory> & categories, ObjectConfig::EObjectCategory category)
{
	return vstd::contains(categories, category);
}

/// Ban evaluation of `bannedObjects` only (exact, then type-level). Categories are ignored.
bool bannedObjectsResolve(const std::map<CompoundMapObjectID, bool> & bans, const CompoundMapObjectID & objid)
{
	auto exact = bans.find(objid);
	if(exact != bans.end())
		return exact->second;

	if(objid.secondaryID != -1)
	{
		auto type = bans.find(CompoundMapObjectID(objid.primaryID, -1));
		if(type != bans.end())
			return type->second;
	}

	return false;
}

}

void ObjectConfig::setObjectBanned(const CompoundMapObjectID & objid, bool banned)
{
	bannedObjects[objid] = banned;

	logGlobal->debug("%s object of type %d.%d", banned ? "Banned" : "Allowed", objid.primaryID, objid.secondaryID);
}

void ObjectConfig::addBannedCategory(EObjectCategory category)
{
	bannedObjectCategories.push_back(category);
}

void ObjectConfig::addCustomObject(const ObjectInfo & object, const CObjectClassesHandler & objects)
{
	customObjects.push_back(object);
	auto & lastObject = customObjects.back();
	lastObject.setAllTemplates(objects, object.primaryID, object.secondaryID);

	assert(lastObject.templates.size() > 0);
	logGlobal->debug("Added custom object of type %d.%d", object.primaryID, object.secondaryID);
}

void ObjectConfig::addRequiredObject(const CompoundMapObjectID & objid, ui16 count, std::optional<ui32> guardLevel)
{
	requiredObjects[objid] = std::pair{count, guardLevel};

	logGlobal->debug("Added required object of type %d.%d, count: %d, guard level: %d",
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

void ObjectConfig::serializeJson(JsonSerializeFormat & handler, const CObjectClassesHandler & objects)
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
				addBannedCategory(it->second);
		}
	}

	if (handler.saving)
	{
		std::map<int, std::map<int, bool>> groupedBanned;
		for(const auto & [objid, banned] : bannedObjects)
		{
			groupedBanned[objid.primaryID][objid.secondaryID] = banned;
		}
		
		auto bannedObjectsStruct = handler.enterStruct("bannedObjects");
		
		for(const auto & [primaryID, secondaryIDs] : groupedBanned)
		{
			const std::string jsonKey = objects.getJsonKey(primaryID);
			
			if(secondaryIDs.size() == 1 && secondaryIDs.begin()->first == -1)
			{
				JsonNode flag(secondaryIDs.begin()->second);
				bannedObjectsStruct->serializeRaw(jsonKey, flag, std::nullopt);
			}
			else
			{
				auto objStruct = bannedObjectsStruct->enterStruct(jsonKey);
				for(const auto & [secondaryID, banned] : secondaryIDs)
				{
					if(secondaryID != -1)
					{
						auto objectHandler = objects.getHandlerFor(MapObjectID(primaryID), MapObjectSubID(secondaryID));
						const std::string subtypeKey = objectHandler->getSubTypeName();
						JsonNode flag(banned);
						objStruct->serializeRaw(subtypeKey, flag, std::nullopt);
					}
				}
			}
		}
	}
	else
	{
		const JsonNode & config = handler.getCurrent();
		const JsonNode & configBannedObjects = config["bannedObjects"];
		auto & resolver = handler.getIdentifierResolver();

		for(const auto & node : configBannedObjects.Struct())
		{
			resolver.requestIdentifierIfFound(node.second.getModScope(), "object", node.first, [this, node, &resolver, &objects](int primaryID)
			{
				if (node.second.isBool())
				{
					setObjectBanned(CompoundMapObjectID(primaryID, -1), node.second.Bool());
				}
				else
				{
					for (const auto & subNode : node.second.Struct())
					{
						const std::string jsonKey = objects.getJsonKey(primaryID);

						resolver.requestIdentifierIfFound(node.second.getModScope(), jsonKey, subNode.first, [this, primaryID, subNode](int secondaryID)
						{
							setObjectBanned(CompoundMapObjectID(primaryID, secondaryID), subNode.second.Bool());
						});
					}
				}
			});
		}
	}

	if (handler.saving)
	{
		auto commonObjectsArray = handler.enterArray("commonObjects");
		commonObjectsArray.syncSize(customObjects, JsonNode::JsonType::DATA_STRUCT);
		
		for(size_t i = 0; i < customObjects.size(); ++i)
		{
			auto objectStruct = commonObjectsArray.enterStruct(i);
			const auto & object = customObjects[i];
			
			std::string objectType = objects.getJsonKey(object.primaryID);
			objectStruct->serializeString("type", objectType);
			
			auto handler = objects.getHandlerFor(MapObjectID(object.primaryID), MapObjectSubID(object.secondaryID));
			std::string subtypeName = handler->getSubTypeName();
			objectStruct->serializeString("subtype", subtypeName);
			
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
		auto & resolver = handler.getIdentifierResolver();

		for (const auto & objectConfig : configCommonObjects.Vector())
		{
			if (!objectConfig["type"].isString() || objectConfig["type"].String().empty())
			{
				logMod->error("Mod '%s': RMG commonObjects entry without \"type\" is ignored: %s", objectConfig.getModScope(), objectConfig.toCompactString());
				continue;
			}

			auto rmg = objectConfig["rmg"].Struct();

			ObjectInfo object;

			object.value = rmg["value"].Integer();
			object.probability = rmg["rarity"].Integer();
			object.maxPerZone = rmg["zoneLimit"].Integer();
			if (object.maxPerZone == 0)
				object.maxPerZone = std::numeric_limits<int>::max();

			const bool hasRarity = !rmg["rarity"].isNull();
			auto addObject = [this, hasRarity, &objects](ObjectInfo objectWithID)
			{
				if (!hasRarity && objectWithID.primaryID == Obj::RANDOM_DWELLING_LVL)
					objectWithID.probability = NATIVE_DWELLING_RARITY;
				addCustomObject(objectWithID, objects);
			};

			resolver.requestIdentifierIfFound("object", objectConfig["type"], [addObject, object, objectConfig, &resolver, &objects](int primaryID)
			{
				if (objectConfig["subtype"].isNull())
				{
					auto objectWithID = object;
					objectWithID.primaryID = primaryID;
					objectWithID.secondaryID = 0;
					addObject(objectWithID);
				}
				else
				{
					const std::string jsonKey = objects.getJsonKey(primaryID);

					resolver.requestIdentifierIfFound(jsonKey, objectConfig["subtype"], [addObject, primaryID, object](int secondaryID)
					{
						auto objectWithID = object;
						objectWithID.primaryID = primaryID;
						objectWithID.secondaryID = secondaryID;
						addObject(objectWithID);
					});
				}
			});
		}
	}

	if(handler.saving)
	{
		if(!requiredObjects.empty())
		{
			auto requiredObjectsStruct = handler.enterStruct("requiredObjects");
			for(const auto & [objId, info] : requiredObjects)
			{
				std::string jsonKey = objects.getJsonKey(objId.primaryID);
				auto entryStruct = requiredObjectsStruct->enterStruct(jsonKey);
				auto objHandler = objects.getHandlerFor(objId);
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
		auto & resolver = handler.getIdentifierResolver();

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
			resolver.requestIdentifierIfFound(node.second.getModScope(), "object", node.first, [this, node, getInfo, &resolver, &objects](int primaryID)
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
						for (const auto & subNode : node.second.Struct())
						{
							std::string jsonKey = objects.getJsonKey(primaryID);
							if(node.first == "artifact" || node.first == "core:artifact")
								jsonKey = "artifact";

							resolver.requestIdentifierIfFound(node.second.getModScope(), jsonKey, subNode.first, [this, primaryID, subNode, getInfo](int secondaryID)
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

const std::map<CompoundMapObjectID, bool> & ObjectConfig::getObjectBans() const
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

bool ObjectConfig::isObjectBanned(const CompoundMapObjectID & objid) const
{
	auto exact = bannedObjects.find(objid);
	if(exact != bannedObjects.end())
		return exact->second;

	if(objid.secondaryID != -1)
	{
		auto type = bannedObjects.find(CompoundMapObjectID(objid.primaryID, -1));
		if(type != bannedObjects.end())
			return type->second;
	}

	if(categoryContains(bannedObjectCategories, EObjectCategory::ALL))
		return true;

	auto category = getObjectCategory(objid);
	if(category != EObjectCategory::NONE && categoryContains(bannedObjectCategories, category))
		return true;

	return false;
}

ObjectConfig ObjectConfig::merge(const ObjectConfig & base, const ObjectConfig & override)
{
	ObjectConfig result = base;

	for(auto category : override.bannedObjectCategories)
	{
		if(!categoryContains(result.bannedObjectCategories, category))
			result.bannedObjectCategories.push_back(category);
	}

	for(const auto & [id, banned] : override.bannedObjects)
		result.bannedObjects[id] = banned;

	std::set<CompoundMapObjectID> overrideIds;
	for(const auto & object : override.customObjects)
		overrideIds.insert(object.getCompoundID());

	std::vector<ObjectInfo> mergedCustom;
	for(const auto & object : base.customObjects)
	{
		auto id = object.getCompoundID();
		if(overrideIds.count(id) || bannedObjectsResolve(override.bannedObjects, id))
			continue;
		mergedCustom.push_back(object);
	}
	for(const auto & object : override.customObjects)
		mergedCustom.push_back(object);
	result.customObjects = std::move(mergedCustom);

	for(const auto & [id, info] : override.requiredObjects)
		result.requiredObjects[id] = info;

	return result;
}
