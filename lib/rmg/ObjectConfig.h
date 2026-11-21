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
#include "../json/JsonNode.h"
#include "ObjectInfo.h"

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
	/// Entry of customObjects.pandoraBoxes: Pandora box map options plus an "rmg" block, parsed by TreasurePlacer during generation
	void addPandoraBox(const JsonNode & entry);

	struct SpellScrollInfo
	{
		SpellID spell;
		std::optional<ui32> value;
		std::optional<ui16> rarity;
		ui32 zoneLimit = 0; // 0 = unlimited
	};

	void addSpellScroll(const SpellScrollInfo & scroll);
	const std::vector<SpellScrollInfo> & getSpellScrolls() const;
	void addRequiredScroll(SpellID spell, ui16 count, std::optional<ui32> guardLevel = std::nullopt);
	const std::map<SpellID, std::pair<ui16, std::optional<ui32>>> & getRequiredScrolls() const;

	/// Built-in value of a scroll with this spell: scrollValues[level - 1], or 0 when the level has no entry
	static ui32 getDefaultScrollValue(SpellID spell, const std::vector<int> & scrollValues);

	const std::map<CompoundMapObjectID, bool> & getObjectBans() const;
	const std::vector<EObjectCategory> & getBannedObjectCategories() const;
	const std::vector<ObjectInfo> & getConfiguredObjects() const;
	const std::vector<JsonNode> & getPandoraBoxes() const;
	bool isObjectBanned(const CompoundMapObjectID & objid) const;

	std::map<CompoundMapObjectID, std::pair<ui16, std::optional<ui32>>> getRequiredObjects() const;

	/// Effective config of a zone: `override` entries replace `base` entries per object, per the VCM-22 merge rules
	static ObjectConfig merge(const ObjectConfig & base, const ObjectConfig & override);

	void serializeJson(JsonSerializeFormat & handler, const CObjectClassesHandler & objects);
private:
	std::map<CompoundMapObjectID, bool> bannedObjects;
	std::vector<EObjectCategory> bannedObjectCategories;

	std::vector<ObjectInfo> customObjects;
	std::map<CompoundMapObjectID, std::pair<ui16, std::optional<ui32>>> requiredObjects; // obligatory, potentially guarded objects to spawn in this zone
	std::vector<JsonNode> pandoraBoxes; // raw JSON, because rewards cannot be loaded while templates are (LIBRARY is not set yet)
	std::vector<SpellScrollInfo> spellScrolls;
	std::map<SpellID, std::pair<ui16, std::optional<ui32>>> requiredScrolls;

};
