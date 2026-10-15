/*
 * EditorMapHelpers.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorMapHelpers.h"

#include "EditorMap.h"
#include "EditorTexts.h"

#include "../GameEngine.h"
#include "../gui/CIntObject.h"
#include "../gui/WindowHandler.h"

#include "../../lib/entities/faction/CFaction.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapping/CMap.h"

namespace EditorMapHelpers
{

int3 posFromJson(const JsonNode & json)
{
	return int3(json.Vector()[0].Integer(), json.Vector()[1].Integer(), json.Vector()[2].Integer());
}

std::vector<JsonNode> linearJsonArray(const JsonNode & json)
{
	std::vector<JsonNode> result;
	if(json.getType() == JsonNode::JsonType::DATA_STRUCT)
		result.push_back(json);
	if(json.getType() == JsonNode::JsonType::DATA_VECTOR)
	{
		for(auto & node : json.Vector())
		{
			auto subvector = linearJsonArray(node);
			result.insert(result.end(), subvector.begin(), subvector.end());
		}
	}
	return result;
}

std::string getTownName(const CMap & map, int objectIdx)
{
	std::string name;
	if(auto town = dynamic_cast<const CGTownInstance*>(map.objects.at(objectIdx).get()))
	{
		name = EditorTexts::translate(TextIdentifier(town->getNameTextID()));

		if(name.empty())
			name = town->getTown()->faction->getNameTranslated();
	}
	return name;
}

std::string getHeroName(const CMap & map, int objectIdx)
{
	std::string name;
	if(auto hero = dynamic_cast<const CGHeroInstance*>(map.objects.at(objectIdx).get()))
	{
		name = EditorTexts::translate(TextIdentifier(hero->getNameTextID()));
	}
	else if(auto placeholder = dynamic_cast<const CGHeroPlaceholder*>(map.objects.at(objectIdx).get()))
	{
		if(placeholder->heroType.has_value())
			name = placeholder->heroType->toHeroType()->getNameTranslated();
		else if(placeholder->powerRank.has_value())
			name = EditorTexts::format("hero.placeholderRank", {std::to_string(placeholder->powerRank.value())});
		else
			// Hero placeholders are expected to be initialized with either heroType or powerRank.
			assert(0);
	}
	return name;
}

std::string getMonsterName(const CMap & map, int objectIdx)
{
	std::string name;
	if(auto monster = dynamic_cast<const CGCreature*>(map.objects.at(objectIdx).get()))
		name = EditorTexts::translate(monster->getObjectName()) + " @ " + monster->anchorPos().toString();

	return name;
}

std::vector<int> getHeroTargetObjectIndexes(const CMap & map)
{
	std::vector<int> result = getObjectIndexes<const CGHeroInstance>(map);
	for(const auto * placeholder : map.getObjects<CGHeroPlaceholder>())
		result.push_back(placeholder->id.getNum());

	return result;
}

int getHeroTargetObjectByPos(const CMap & map, const int3 & pos)
{
	for(const auto * hero : map.getObjects<CGHeroInstance>())
	{
		const bool matchesPosition =
			hero->anchorPos() == pos ||
			(hero->isVisitable() && hero->visitableAt(pos)) ||
			hero->coveringAt(pos);

		if(matchesPosition)
			return hero->id.getNum();
	}

	for(const auto * placeholder : map.getObjects<CGHeroPlaceholder>())
	{
		const bool matchesPosition =
			placeholder->anchorPos() == pos ||
			(placeholder->isVisitable() && placeholder->visitableAt(pos)) ||
			placeholder->coveringAt(pos);

		if(matchesPosition)
			return placeholder->id.getNum();
	}

	return -1;
}

bool isHeroTargetObject(const CGObjectInstance * obj)
{
	return dynamic_cast<const CGHeroInstance *>(obj) || dynamic_cast<const CGHeroPlaceholder *>(obj);
}

JsonNode conditionToJson(const EventCondition & event)
{
	JsonNode result;
	result["condition"].Integer() = event.condition;
	result["value"].Integer() = event.value;
	result["objectType"].String() = event.objectType.toString();
	result["objectInstanceName"].String() = event.objectInstanceName;
	{
		auto & position = result["position"].Vector();
		position.resize(3);
		position[0].Float() = event.position.x;
		position[1].Float() = event.position.y;
		position[2].Float() = event.position.z;
	}
	return result;
}

void pickObject(EditorMap & editor, int windowsToHide, const PickFilter & filter, const PickCallback & callback)
{
	if(!editor.pickObject)
		return;

	auto hidden = std::make_shared<std::vector<std::shared_ptr<IShowActivatable>>>();
	for(int i = 0; i < windowsToHide; ++i)
	{
		auto window = ENGINE->windows().topWindow<IShowActivatable>();
		if(!window)
			break;

		ENGINE->windows().popWindow(window);
		hidden->push_back(window);
	}

	editor.pickObject(filter, [hidden, callback](const CGObjectInstance * object)
	{
		for(auto window = hidden->rbegin(); window != hidden->rend(); ++window)
			ENGINE->windows().pushWindow(*window);

		if(callback)
			callback(object);
	});
}

std::string defaultBuildingName(const BuildingID & id)
{
	static const std::map<BuildingID, std::string> names = {
		{BuildingID::DEFAULT, "DEFAULT"},
		{BuildingID::MAGES_GUILD_1, "MAGES_GUILD_1"},
		{BuildingID::MAGES_GUILD_2, "MAGES_GUILD_2"},
		{BuildingID::MAGES_GUILD_3, "MAGES_GUILD_3"},
		{BuildingID::MAGES_GUILD_4, "MAGES_GUILD_4"},
		{BuildingID::MAGES_GUILD_5, "MAGES_GUILD_5"},
		{BuildingID::TAVERN, "TAVERN"},
		{BuildingID::SHIPYARD, "SHIPYARD"},
		{BuildingID::FORT, "FORT"},
		{BuildingID::CITADEL, "CITADEL"},
		{BuildingID::CASTLE, "CASTLE"},
		{BuildingID::VILLAGE_HALL, "VILLAGE_HALL"},
		{BuildingID::TOWN_HALL, "TOWN_HALL"},
		{BuildingID::CITY_HALL, "CITY_HALL"},
		{BuildingID::CAPITOL, "CAPITOL"},
		{BuildingID::MARKETPLACE, "MARKETPLACE"},
		{BuildingID::RESOURCE_SILO, "RESOURCE_SILO"},
		{BuildingID::BLACKSMITH, "BLACKSMITH"},
		{BuildingID::SPECIAL_1, "SPECIAL_1"},
		{BuildingID::SPECIAL_2, "SPECIAL_2"},
		{BuildingID::SPECIAL_3, "SPECIAL_3"},
		{BuildingID::SPECIAL_4, "SPECIAL_4"},
		{BuildingID::HORDE_1, "HORDE_1"},
		{BuildingID::HORDE_1_UPGR, "HORDE_1_UPGR"},
		{BuildingID::HORDE_2, "HORDE_2"},
		{BuildingID::HORDE_2_UPGR, "HORDE_2_UPGR"},
		{BuildingID::SHIP, "SHIP"},
		{BuildingID::GRAIL, "GRAIL"},
		{BuildingID::EXTRA_TOWN_HALL, "EXTRA_TOWN_HALL"},
		{BuildingID::EXTRA_CITY_HALL, "EXTRA_CITY_HALL"},
		{BuildingID::EXTRA_CAPITOL, "EXTRA_CAPITOL"},
		{BuildingID::DWELL_LVL_1, "DWELL_LVL_1"},
		{BuildingID::DWELL_LVL_2, "DWELL_LVL_2"},
		{BuildingID::DWELL_LVL_3, "DWELL_LVL_3"},
		{BuildingID::DWELL_LVL_4, "DWELL_LVL_4"},
		{BuildingID::DWELL_LVL_5, "DWELL_LVL_5"},
		{BuildingID::DWELL_LVL_6, "DWELL_LVL_6"},
		{BuildingID::DWELL_LVL_7, "DWELL_LVL_7"},
		{BuildingID::DWELL_LVL_8, "DWELL_LVL_8"},
		{BuildingID::DWELL_LVL_1_UP, "DWELL_LVL_1_UP"},
		{BuildingID::DWELL_LVL_2_UP, "DWELL_LVL_2_UP"},
		{BuildingID::DWELL_LVL_3_UP, "DWELL_LVL_3_UP"},
		{BuildingID::DWELL_LVL_4_UP, "DWELL_LVL_4_UP"},
		{BuildingID::DWELL_LVL_5_UP, "DWELL_LVL_5_UP"},
		{BuildingID::DWELL_LVL_6_UP, "DWELL_LVL_6_UP"},
		{BuildingID::DWELL_LVL_7_UP, "DWELL_LVL_7_UP"},
		{BuildingID::DWELL_LVL_8_UP, "DWELL_LVL_8_UP"},
	};

	auto it = names.find(id);
	return it == names.end() ? "UNKNOWN" : it->second;
}

std::string describeObject(const CGObjectInstance * object)
{
	return EditorTexts::translate(object->getObjectName()) + ", x: " + std::to_string(object->pos.x) + ", y: " + std::to_string(object->pos.y) + ", z: " + std::to_string(object->pos.z);
}

}
