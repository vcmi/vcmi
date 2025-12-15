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

#include "../mapObjects/CompoundMapObjectID.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE ObjectConfig
{
#ifdef ENABLE_TEMPLATE_EDITOR
	friend class ObjectSelector;
#endif

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

VCMI_LIB_NAMESPACE_END
