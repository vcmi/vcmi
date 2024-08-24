/*
 * ObjectInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../mapObjects/ObjectTemplate.h"
#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CompoundMapObjectID;
class CGObjectInstance;

struct DLL_LINKAGE ObjectInfo
{
	ObjectInfo();
	ObjectInfo(const ObjectInfo & other);
	ObjectInfo & operator=(const ObjectInfo & other);

	std::vector<std::shared_ptr<const ObjectTemplate>> templates;
	ui32 value = 0;
	ui16 probability = 0;
	ui32 maxPerZone = 1;
	//ui32 maxPerMap; //unused
	std::function<CGObjectInstance *()> generateObject;
	std::function<void(CGObjectInstance *)> destroyObject;
	
	void setTemplates(MapObjectID type, MapObjectSubID subtype, TerrainId terrain);

	//bool matchesId(const CompoundMapObjectID & id) const;
};

// TODO: Store config for custom objects to spawn in this zone
// TODO: Read custom object config from zone file
class DLL_LINKAGE ObjectConfig
{
public:

	enum class EObjectCategory
	{
		OTHER = -2,
		ALL = -1,
		NONE = 0,
		CREATURE_BANK = 1,
		BONUS,
		DWELLING,
		RESOURCE,
		RESOURCE_GENERATOR,
		SPELL_SCROLL,
		RANDOM_ARTIFACT,
		PANDORAS_BOX,
		QUEST_ARTIFACT,
		SEER_HUT
	};

	void addBannedObject(const CompoundMapObjectID & objid);
	void addCustomObject(const ObjectInfo & object);
	void clearBannedObjects();
	void clearCustomObjects();
	const std::vector<CompoundMapObjectID> & getBannedObjects() const;
	const std::vector<EObjectCategory> & getBannedObjectCategories() const;
	const std::vector<ObjectInfo> & getConfiguredObjects() const;

	void serializeJson(JsonSerializeFormat & handler);
private:
	// TODO: Add convenience method for banning objects by name
	std::vector<CompoundMapObjectID> bannedObjects;
	std::vector<EObjectCategory> bannedObjectCategories;

	// TODO: In what format should I store custom objects?
	// Need to convert map serialization format to ObjectInfo
	std::vector<ObjectInfo> customObjects;
};
// TODO: Allow to copy all custom objects config from another zone

VCMI_LIB_NAMESPACE_END