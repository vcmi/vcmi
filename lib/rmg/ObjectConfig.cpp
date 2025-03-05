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

void ObjectConfig::addCustomObject(const ObjectInfo & object, const CompoundMapObjectID & objid)
{
	customObjects.push_back(object);
	auto & lastObject = customObjects.back();
	lastObject.setAllTemplates(objid.primaryID, objid.secondaryID);

	assert(lastObject.templates.size() > 0);
	logGlobal->info("Added custom object of type %d.%d", objid.primaryID, objid.secondaryID);
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


	// TODO: Separate into individual methods to enforce RAII destruction?
	{
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
	}

	// FIXME: Doesn't seem to use this field at all
	
	{
		auto bannedObjectData = handler.enterArray("bannedObjects");	
		if (handler.saving)
		{

			// FIXME: Do we even need to serialize / store banned objects?
			/*
			for (const auto & object : bannedObjects)
			{
				// TODO: Translate id back to string?


				JsonNode node;
				node.String() = LIBRARY->objtypeh->getHandlerFor(object.primaryID, object.secondaryID);
				// TODO: Check if AI-generated code is right


			}
			// handler.serializeRaw("bannedObjects", node, std::nullopt);

			*/
		}
		else
		{
			std::vector<std::string> objectNames;
			bannedObjectData.serializeArray(objectNames);

			for (const auto & objectName : objectNames)
			{
				LIBRARY->objtypeh->resolveObjectCompoundId(objectName,
					[this](CompoundMapObjectID objid)
					{
						addBannedObject(objid);
					}
				);
				
			}
		}
	}

	auto commonObjectData = handler.getCurrent()["commonObjects"].Vector();	
	if (handler.saving)
	{

		//TODO?
	}
	else
	{
		for (const auto & objectConfig : commonObjectData)
		{
			auto objectName = objectConfig["id"].String();
			auto rmg = objectConfig["rmg"].Struct();

			// TODO: Use common code with default rmg config
			auto objectValue = rmg["value"].Integer();
			auto objectProbability = rmg["rarity"].Integer();

			auto objectMaxPerZone = rmg["zoneLimit"].Integer();
			if (objectMaxPerZone == 0)
			{
				objectMaxPerZone = std::numeric_limits<int>::max();
			}

			LIBRARY->objtypeh->resolveObjectCompoundId(objectName,

				[this, objectValue, objectProbability, objectMaxPerZone](CompoundMapObjectID objid)
				{
					ObjectInfo object(objid.primaryID, objid.secondaryID);
					
					// TODO: Configure basic generateObject function

					object.value = objectValue;
					object.probability = objectProbability;
					object.maxPerZone = objectMaxPerZone;
					addCustomObject(object, objid);
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