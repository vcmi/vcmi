/*
 * EditorMapHelpers.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/mapping/CMap.h"

#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/int3.h"
#include "../../lib/json/JsonNode.h"

class CMap;
class CGObjectInstance;
class EditorMap;
struct EventCondition;

/// Names and lookups of the map objects that are used by many windows of the editor
namespace EditorMapHelpers
{
	int3 posFromJson(const JsonNode & json);
	std::vector<JsonNode> linearJsonArray(const JsonNode & json);

	std::string getTownName(const CMap & map, int objectIdx);
	std::string getHeroName(const CMap & map, int objectIdx);
	std::string getMonsterName(const CMap & map, int objectIdx);
	std::vector<int> getHeroTargetObjectIndexes(const CMap & map);
	int getHeroTargetObjectByPos(const CMap & map, const int3 & pos);
	bool isHeroTargetObject(const CGObjectInstance * obj);

	JsonNode conditionToJson(const EventCondition & event);

	/// Indexes of the objects of the type
	template<class T>
	std::vector<int> getObjectIndexes(const CMap & map)
	{
		std::vector<int> result;
		for(const auto & obj : map.getObjects<T>())
			result.push_back(obj->id.getNum());

		return result;
	}

	/// Index of the object of the type at the position, or -1
	template<class T>
	int getObjectByPos(const CMap & map, const int3 & pos)
	{
		for(const auto & obj : map.getObjects<T>())
			if(obj->pos == pos)
				return obj->id.getNum();

		return -1;
	}

	/// Identifier of the building that is shown when the building has no name
	std::string defaultBuildingName(const BuildingID & id);

	/// Text with the name of the object and its position
	std::string describeObject(const CGObjectInstance * object);

	using PickFilter = std::function<bool(const CGObjectInstance *)>;
	using PickCallback = std::function<void(const CGObjectInstance *)>;

	/// Hides the topmost windows, lets the user click an object on the map and shows the windows again.
	/// The callback gets nullptr if the user clicks something else
	void pickObject(EditorMap & editor, int windowsToHide, const PickFilter & filter, const PickCallback & callback);
}
