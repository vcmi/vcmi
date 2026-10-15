/*
 * EditorObjectCatalog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorObjectCatalog.h"

#include "EditorTexts.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapObjects/ObjectTemplate.h"

#include <boost/algorithm/string/predicate.hpp>

namespace
{
enum class Group
{
	TOWNS,
	OBJECTS,
	HEROES,
	ARTIFACTS,
	RESOURCES,
	BANKS,
	DWELLINGS,
	GROUNDS,
	TELEPORTS,
	MINES,
	TRIGGERS,
	MONSTERS,
	QUESTS,
	WOG_OBJECTS,
	OBSTACLES,
	OTHER
};

struct CatalogEntry
{
	Group group;
	/// Objects of the type are named after the type instead of the sub-type name
	bool useObjectName;
	MapObjectID id;
};

const std::vector<CatalogEntry> & catalogEntries()
{
	static const std::vector<CatalogEntry> entries = {
		{Group::TOWNS, false, Obj::TOWN},
		{Group::TOWNS, false, Obj::RANDOM_TOWN},
		{Group::TOWNS, true, Obj::SHIPYARD},
		{Group::TOWNS, true, Obj::GARRISON},
		{Group::TOWNS, true, Obj::GARRISON2},
		{Group::OBJECTS, true, Obj::ARENA},
		{Group::OBJECTS, true, Obj::BUOY},
		{Group::OBJECTS, true, Obj::CARTOGRAPHER},
		{Group::OBJECTS, true, Obj::SWAN_POND},
		{Group::OBJECTS, true, Obj::COVER_OF_DARKNESS},
		{Group::OBJECTS, true, Obj::CORPSE},
		{Group::OBJECTS, true, Obj::FAERIE_RING},
		{Group::OBJECTS, true, Obj::FOUNTAIN_OF_FORTUNE},
		{Group::OBJECTS, true, Obj::FOUNTAIN_OF_YOUTH},
		{Group::OBJECTS, true, Obj::GARDEN_OF_REVELATION},
		{Group::OBJECTS, true, Obj::HILL_FORT},
		{Group::OBJECTS, true, Obj::IDOL_OF_FORTUNE},
		{Group::OBJECTS, true, Obj::LIBRARY_OF_ENLIGHTENMENT},
		{Group::OBJECTS, true, Obj::LIGHTHOUSE},
		{Group::OBJECTS, true, Obj::SCHOOL_OF_MAGIC},
		{Group::OBJECTS, true, Obj::MAGIC_SPRING},
		{Group::OBJECTS, true, Obj::MAGIC_WELL},
		{Group::OBJECTS, true, Obj::MERCENARY_CAMP},
		{Group::OBJECTS, true, Obj::MERMAID},
		{Group::OBJECTS, true, Obj::MYSTICAL_GARDEN},
		{Group::OBJECTS, true, Obj::OASIS},
		{Group::OBJECTS, true, Obj::LEAN_TO},
		{Group::OBJECTS, true, Obj::OBELISK},
		{Group::OBJECTS, true, Obj::REDWOOD_OBSERVATORY},
		{Group::OBJECTS, true, Obj::PILLAR_OF_FIRE},
		{Group::OBJECTS, true, Obj::STAR_AXIS},
		{Group::OBJECTS, true, Obj::RALLY_FLAG},
		{Group::OBJECTS, true, Obj::WATERING_HOLE},
		{Group::OBJECTS, true, Obj::SCHOLAR},
		{Group::OBJECTS, true, Obj::SHRINE_OF_MAGIC_INCANTATION},
		{Group::OBJECTS, true, Obj::SHRINE_OF_MAGIC_GESTURE},
		{Group::OBJECTS, true, Obj::SHRINE_OF_MAGIC_THOUGHT},
		{Group::OBJECTS, true, Obj::SIRENS},
		{Group::OBJECTS, true, Obj::STABLES},
		{Group::OBJECTS, true, Obj::TAVERN},
		{Group::OBJECTS, true, Obj::TEMPLE},
		{Group::OBJECTS, true, Obj::DEN_OF_THIEVES},
		{Group::OBJECTS, true, Obj::LEARNING_STONE},
		{Group::OBJECTS, true, Obj::TREE_OF_KNOWLEDGE},
		{Group::OBJECTS, true, Obj::WAGON},
		{Group::OBJECTS, true, Obj::SCHOOL_OF_WAR},
		{Group::OBJECTS, true, Obj::WAR_MACHINE_FACTORY},
		{Group::OBJECTS, true, Obj::WARRIORS_TOMB},
		{Group::OBJECTS, true, Obj::WITCH_HUT},
		{Group::OBJECTS, true, Obj::SANCTUARY},
		{Group::OBJECTS, true, Obj::MARLETTO_TOWER},
		{Group::HEROES, true, Obj::PRISON},
		{Group::HEROES, false, Obj::HERO},
		{Group::HEROES, false, Obj::RANDOM_HERO},
		{Group::HEROES, false, Obj::HERO_PLACEHOLDER},
		{Group::HEROES, false, Obj::BOAT},
		{Group::ARTIFACTS, true, Obj::ARTIFACT},
		{Group::ARTIFACTS, false, Obj::RANDOM_ART},
		{Group::ARTIFACTS, false, Obj::RANDOM_TREASURE_ART},
		{Group::ARTIFACTS, false, Obj::RANDOM_MINOR_ART},
		{Group::ARTIFACTS, false, Obj::RANDOM_MAJOR_ART},
		{Group::ARTIFACTS, false, Obj::RANDOM_RELIC_ART},
		{Group::ARTIFACTS, true, Obj::SPELL_SCROLL},
		{Group::ARTIFACTS, true, Obj::PANDORAS_BOX},
		{Group::RESOURCES, true, Obj::RANDOM_RESOURCE},
		{Group::RESOURCES, false, Obj::RESOURCE},
		{Group::RESOURCES, true, Obj::SEA_CHEST},
		{Group::RESOURCES, true, Obj::TREASURE_CHEST},
		{Group::RESOURCES, true, Obj::CAMPFIRE},
		{Group::RESOURCES, true, Obj::SHIPWRECK_SURVIVOR},
		{Group::RESOURCES, true, Obj::FLOTSAM},
		{Group::BANKS, true, Obj::CREATURE_BANK},
		{Group::BANKS, true, Obj::DRAGON_UTOPIA},
		{Group::BANKS, true, Obj::CRYPT},
		{Group::BANKS, true, Obj::DERELICT_SHIP},
		{Group::BANKS, true, Obj::PYRAMID},
		{Group::BANKS, true, Obj::SHIPWRECK},
		{Group::DWELLINGS, true, Obj::CREATURE_GENERATOR1},
		{Group::DWELLINGS, true, Obj::CREATURE_GENERATOR2},
		{Group::DWELLINGS, true, Obj::CREATURE_GENERATOR3},
		{Group::DWELLINGS, true, Obj::CREATURE_GENERATOR4},
		{Group::DWELLINGS, true, Obj::REFUGEE_CAMP},
		{Group::DWELLINGS, false, Obj::RANDOM_DWELLING},
		{Group::DWELLINGS, false, Obj::RANDOM_DWELLING_LVL},
		{Group::DWELLINGS, false, Obj::RANDOM_DWELLING_FACTION},
		{Group::GROUNDS, true, Obj::CURSED_GROUND1},
		{Group::GROUNDS, true, Obj::MAGIC_PLAINS1},
		{Group::GROUNDS, true, Obj::CLOVER_FIELD},
		{Group::GROUNDS, true, Obj::CURSED_GROUND2},
		{Group::GROUNDS, true, Obj::EVIL_FOG},
		{Group::GROUNDS, true, Obj::FAVORABLE_WINDS},
		{Group::GROUNDS, true, Obj::FIERY_FIELDS},
		{Group::GROUNDS, true, Obj::HOLY_GROUNDS},
		{Group::GROUNDS, true, Obj::LUCID_POOLS},
		{Group::GROUNDS, true, Obj::MAGIC_CLOUDS},
		{Group::GROUNDS, true, Obj::MAGIC_PLAINS2},
		{Group::GROUNDS, true, Obj::ROCKLANDS},
		{Group::GROUNDS, true, Obj::HOLE},
		{Group::TELEPORTS, true, Obj::MONOLITH_ONE_WAY_ENTRANCE},
		{Group::TELEPORTS, true, Obj::MONOLITH_ONE_WAY_EXIT},
		{Group::TELEPORTS, true, Obj::MONOLITH_TWO_WAY},
		{Group::TELEPORTS, true, Obj::SUBTERRANEAN_GATE},
		{Group::TELEPORTS, true, Obj::WHIRLPOOL},
		{Group::MINES, true, Obj::MINE},
		{Group::MINES, false, Obj::ABANDONED_MINE},
		{Group::MINES, true, Obj::WINDMILL},
		{Group::MINES, true, Obj::WATER_WHEEL},
		{Group::TRIGGERS, true, Obj::EVENT},
		{Group::TRIGGERS, true, Obj::GRAIL},
		{Group::TRIGGERS, true, Obj::SIGN},
		{Group::TRIGGERS, true, Obj::OCEAN_BOTTLE},
		{Group::MONSTERS, false, Obj::MONSTER},
		{Group::MONSTERS, true, Obj::RANDOM_MONSTER},
		{Group::MONSTERS, true, Obj::RANDOM_MONSTER_L1},
		{Group::MONSTERS, true, Obj::RANDOM_MONSTER_L2},
		{Group::MONSTERS, true, Obj::RANDOM_MONSTER_L3},
		{Group::MONSTERS, true, Obj::RANDOM_MONSTER_L4},
		{Group::MONSTERS, true, Obj::RANDOM_MONSTER_L5},
		{Group::MONSTERS, true, Obj::RANDOM_MONSTER_L6},
		{Group::MONSTERS, true, Obj::RANDOM_MONSTER_L7},
		{Group::QUESTS, true, Obj::SEER_HUT},
		{Group::QUESTS, true, Obj::BORDER_GATE},
		{Group::QUESTS, true, Obj::QUEST_GUARD},
		{Group::QUESTS, true, Obj::HUT_OF_MAGI},
		{Group::QUESTS, true, Obj::EYE_OF_MAGI},
		{Group::QUESTS, true, Obj::BORDERGUARD},
		{Group::QUESTS, true, Obj::KEYMASTER},
		{Group::WOG_OBJECTS, true, Obj::WOG_OBJECT},
	};
	return entries;
}

std::string groupName(Group group)
{
	static const std::map<Group, std::string> keys = {
		{Group::TOWNS, "towns"},
		{Group::OBJECTS, "objects"},
		{Group::HEROES, "heroes"},
		{Group::ARTIFACTS, "artifacts"},
		{Group::RESOURCES, "resources"},
		{Group::BANKS, "banks"},
		{Group::DWELLINGS, "dwellings"},
		{Group::GROUNDS, "grounds"},
		{Group::TELEPORTS, "teleports"},
		{Group::MINES, "mines"},
		{Group::TRIGGERS, "triggers"},
		{Group::MONSTERS, "monsters"},
		{Group::QUESTS, "quests"},
		{Group::WOG_OBJECTS, "wogObjects"},
		{Group::OBSTACLES, "obstacles"},
		{Group::OTHER, "other"},
	};
	return EditorTexts::get("catalog.group." + keys.at(group));
}
}

EditorObjectCatalog::EditorObjectCatalog(IGameInfoCallback * cb)
{
	try
	{
		for(const auto & entry : catalogEntries())
			addGroup(groupName(entry.group), entry.useObjectName, false, entry.id, cb);

		addGroup(groupName(Group::OBSTACLES), true, cb);
		addGroup(groupName(Group::OTHER), false, cb);
	}
	catch(const std::exception & e)
	{
		logGlobal->error("Failed to build the object catalog of the map editor: %s", e.what());
	}
}

EditorObjectCatalog::Node & EditorObjectCatalog::findOrCreateGroup(const std::string & groupName)
{
	for(auto & group : groups)
		if(group.text == groupName)
			return group;

	groups.emplace_back();
	groups.back().text = groupName;
	return groups.back();
}

void EditorObjectCatalog::addGroup(const std::string & groupName, bool staticOnly, IGameInfoCallback * cb)
{
	for(auto id : LIBRARY->objtypeh->knownObjects())
	{
		if(catalogued.count(id))
			continue;

		addGroup(groupName, true, staticOnly, id, cb);
	}
}

void EditorObjectCatalog::addGroup(const std::string & groupName, bool useCustomName, bool staticOnly, MapObjectID id, IGameInfoCallback * cb)
{
	findOrCreateGroup(groupName);

	if(LIBRARY->objtypeh->knownObjects().count(id) == 0)
		return;

	for(auto secondaryID : LIBRARY->objtypeh->knownSubObjects(id))
	{
		auto factory = LIBRARY->objtypeh->getHandlerFor(id, secondaryID);
		auto templates = factory->getTemplates();
		const bool singleTemplate = templates.size() == 1;

		if(staticOnly && !factory->isStaticObject())
			continue;

		// the browser lists types, not placed objects, so the name is resolved without an instance
		auto subGroupTextID = factory->hasNameTextID() ? factory->getNameTextID() : LIBRARY->objtypeh->getObjectGroupNameTextID(id);
		auto subGroupName = EditorTexts::translate(TextIdentifier(subGroupTextID));

		Node typeNode;
		typeNode.text = subGroupName;
		typeNode.typeName = factory->getJsonKey();

		for(size_t templateIndex = 0; templateIndex < templates.size(); ++templateIndex)
		{
			const auto & templ = templates[templateIndex];

			//create object to extract name
			auto temporaryObject = factory->create(cb, templ);
			if(useCustomName)
				typeNode.text = EditorTexts::translate(temporaryObject->getObjectName());

			if(singleTemplate)
			{
				typeNode.id = id;
				typeNode.subId = secondaryID;
				typeNode.templateIndex = templateIndex;
				typeNode.leaf = true;
			}
			else
			{
				Node templateNode;
				templateNode.text = templ->stringID;
				templateNode.typeName = typeNode.typeName;
				templateNode.id = id;
				templateNode.subId = secondaryID;
				templateNode.templateIndex = templateIndex;
				templateNode.leaf = true;
				typeNode.children.push_back(std::move(templateNode));
			}
		}

		findOrCreateGroup(groupName).children.push_back(std::move(typeNode));
		catalogued.insert(id);
	}
}

bool EditorObjectCatalog::matches(const Node & node, const std::string & filter, const TerrainId & terrain, bool parentMatchesText) const
{
	const bool textMatches = filter.empty() || parentMatchesText
		|| boost::algorithm::icontains(node.text, filter)
		|| boost::algorithm::icontains(node.typeName, filter);

	if(!node.leaf)
	{
		for(const auto & child : node.children)
			if(matches(child, filter, terrain, textMatches))
				return true;
		return false;
	}

	if(!textMatches)
		return false;

	if(terrain == ETerrainId::ANY_TERRAIN)
		return true;

	return getTemplate(node)->canBePlacedAt(terrain);
}

void EditorObjectCatalog::collect(const Node & node, int depth, const std::string & filter, const TerrainId & terrain, bool parentMatchesText, std::vector<Row> & rows) const
{
	const bool textMatches = filter.empty() || parentMatchesText
		|| boost::algorithm::icontains(node.text, filter)
		|| boost::algorithm::icontains(node.typeName, filter);

	if(!matches(node, filter, terrain, parentMatchesText))
		return;

	rows.push_back({&node, depth});

	const bool filtered = !filter.empty() || terrain != ETerrainId::ANY_TERRAIN;
	if(!node.leaf && (filtered || isExpanded(node)))
	{
		for(const auto & child : node.children)
			collect(child, depth + 1, filter, terrain, textMatches, rows);
	}
}

std::vector<EditorObjectCatalog::Row> EditorObjectCatalog::getRows(const std::string & filter, const TerrainId & terrain) const
{
	std::vector<Row> rows;
	for(const auto & group : groups)
		collect(group, 0, filter, terrain, false, rows);
	return rows;
}

bool EditorObjectCatalog::isExpanded(const Node & node) const
{
	return expanded.count(&node) != 0;
}

void EditorObjectCatalog::toggleExpanded(const Node & node)
{
	if(!expanded.erase(&node))
		expanded.insert(&node);
}

std::shared_ptr<const ObjectTemplate> EditorObjectCatalog::getTemplate(const Node & leaf)
{
	auto factory = LIBRARY->objtypeh->getHandlerFor(leaf.id, leaf.subId);
	return factory->getTemplates().at(leaf.templateIndex);
}

std::shared_ptr<CGObjectInstance> EditorObjectCatalog::createObject(const Node & leaf, IGameInfoCallback * cb)
{
	auto factory = LIBRARY->objtypeh->getHandlerFor(leaf.id, leaf.subId);
	return factory->create(cb, factory->getTemplates().at(leaf.templateIndex));
}
