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
#include "ObjectInfo.h"
#include "PandoraBoxConfig.h"

class CObjectClassesHandler;
class JsonSerializeFormat;

#ifdef ENABLE_TEMPLATE_EDITOR
class ObjectSelector;
#endif

class DLL_LINKAGE ObjectConfig
{
#ifdef ENABLE_TEMPLATE_EDITOR
	friend class ::ObjectSelector;
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

	/// Rarity of native dwellings without rmg info, also the default rarity of randomDwellingLvl common objects
	static constexpr ui16 NATIVE_DWELLING_RARITY = 40;

	void setObjectBanned(const CompoundMapObjectID & objid, bool banned);
	void addBannedCategory(EObjectCategory category);
	void addCustomObject(const ObjectInfo & object, const CObjectClassesHandler & objects);
	void addRequiredObject(const CompoundMapObjectID & objid, ui16 count, std::optional<ui32> guardLevel = std::nullopt);
	void addRequiredObject(const CompoundMapObjectID & objid, std::pair<ui16, std::optional<ui32>> info);
	void clearRequiredObjects();
	const std::map<CompoundMapObjectID, bool> & getObjectBans() const;
	const std::vector<EObjectCategory> & getBannedObjectCategories() const;
	const std::vector<ObjectInfo> & getConfiguredObjects() const;
	const std::vector<PandoraBoxConfig> & getPandoraBoxes() const;
	void addPandoraBox(const PandoraBoxConfig & box);
	bool isObjectBanned(const CompoundMapObjectID & objid) const;

	std::map<CompoundMapObjectID, std::pair<ui16, std::optional<ui32>>> getRequiredObjects() const;

	/// Effective config of a zone: `override` entries replace `base` entries per object, per the VCM-22 merge rules
	static ObjectConfig merge(const ObjectConfig & base, const ObjectConfig & override);

	void serializeJson(JsonSerializeFormat & handler, const CObjectClassesHandler & objects);
private:
	std::map<CompoundMapObjectID, bool> bannedObjects;
	std::vector<EObjectCategory> bannedObjectCategories;

	std::vector<ObjectInfo> customObjects;
	std::vector<PandoraBoxConfig> pandoraBoxes;
	std::map<CompoundMapObjectID, std::pair<ui16, std::optional<ui32>>> requiredObjects; // obligatory, potentially guarded objects to spawn in this zone

};
