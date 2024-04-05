/*
 * ObstacleSetHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ObstacleSetHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

ObstacleSet::ObstacleSet(EObstacleType type, TerrainId terrain):
	type(type),
	terrain(terrain)
{
}

void ObstacleSet::addObstacle(std::shared_ptr<const ObjectTemplate> obstacle)
{
	obstacles.push_back(obstacle);
}

ObstacleSetFilter::ObstacleSetFilter(std::vector<ObstacleSet::EObstacleType> allowedTypes, TerrainId terrain = TerrainId::ANY_TERRAIN):
	allowedTypes(allowedTypes),
	terrain(terrain)
{
}

ObstacleSetFilter::ObstacleSetFilter(ObstacleSet::EObstacleType allowedType, TerrainId terrain = TerrainId::ANY_TERRAIN):
	allowedTypes({allowedType}),
	terrain(terrain)
{
}

bool ObstacleSetFilter::filter(const ObstacleSet &set) const
{
	return (set.getTerrain() == terrain) || (terrain == TerrainId::ANY_TERRAIN);
}

TerrainId ObstacleSetFilter::getTerrain() const
{
	return terrain;
}

TerrainId ObstacleSet::getTerrain() const
{
	return terrain;
}

ObstacleSet::EObstacleType ObstacleSet::getType() const
{
	return type;
}

std::vector<std::shared_ptr<const ObjectTemplate>> ObstacleSet::getObstacles() const
{
	return obstacles;
}

ObstacleSet::EObstacleType ObstacleSetHandler::convertObstacleClass(MapObjectID id)
{
	switch (id)
	{
		case Obj::MOUNTAIN:
		case Obj::VOLCANIC_VENT:
		case Obj::VOLCANO:
		case Obj::REEF:
			return ObstacleSet::MOUNTAINS;
		case Obj::OAK_TREES:
		case Obj::PINE_TREES:
		case Obj::TREES:
		case Obj::DEAD_VEGETATION:
		case Obj::HEDGE:
		case Obj::KELP:
		case Obj::WILLOW_TREES:
		case Obj::YUCCA_TREES:
			return ObstacleSet::TREES;
		case Obj::FROZEN_LAKE:
		case Obj::LAKE:
		case Obj::LAVA_FLOW:
		case Obj::LAVA_LAKE:
			return ObstacleSet::LAKES;
		case Obj::CANYON:
		case Obj::CRATER:
		case Obj::SAND_PIT:
		case Obj::TAR_PIT:
			return ObstacleSet::CRATERS;
		case Obj::HILL:
		case Obj::MOUND:
		case Obj::OUTCROPPING:
		case Obj::ROCK:
		case Obj::SAND_DUNE:
		case Obj::STALAGMITE:
			return ObstacleSet::ROCKS;
		case Obj::BUSH:
		case Obj::CACTUS:
		case Obj::FLOWERS:
		case Obj::MUSHROOMS:
		case Obj::LOG:
		case Obj::MANDRAKE:
		case Obj::MOSS:
		case Obj::PLANT:
		case Obj::SHRUB:
		case Obj::STUMP:
		case Obj::VINE:
			return ObstacleSet::PLANTS;
		case Obj::SKULL:
			return ObstacleSet::ANIMALS;
		default:
			return ObstacleSet::OTHER;
	}
}

ObstacleSet::EObstacleType ObstacleSet::typeFromString(const std::string &str)
{
	static const std::map<std::string, EObstacleType> OBSTACLE_TYPE_NAMES =
	{
		{"mountain", MOUNTAINS},
		{"tree", TREES},
		{"lake", LAKES},
		{"crater", CRATERS},
		{"rock", ROCKS},
		{"plant", PLANTS},
		{"structure", STRUCTURES},
		{"animal", ANIMALS},
		{"other", OTHER}
	};

	if (OBSTACLE_TYPE_NAMES.find(str) != OBSTACLE_TYPE_NAMES.end())
	{
		return OBSTACLE_TYPE_NAMES.at(str);
	}

	// TODO: How to handle that?
	throw std::runtime_error("Invalid obstacle type: " + str);
}

std::vector<ObstacleSet::EObstacleType> ObstacleSetFilter::getAllowedTypes() const
{
	return allowedTypes;
}

void ObstacleSetHandler::addObstacleSet(const ObstacleSet &os)
{
	obstacleSets[os.getType()].push_back(os);
}

TObstacleTypes ObstacleSetHandler::getObstacles( const ObstacleSetFilter &filter) const
{
	// TODO: Handle multiple terrains for one obstacle set?
	auto terrainType = filter.getTerrain();

	TObstacleTypes result;

	for (const auto &allowedType : filter.getAllowedTypes())
	{
		auto it = obstacleSets.find(allowedType);
		if(it != obstacleSets.end())
		{
			for (const auto &os : it->second)
			{
				if (filter.filter(os))
				{
					result.push_back(os);
				}
			}
		}
	}
	return result;
}

/*
ObstacleSet ObstacleSetHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	// TODO: Merge by name with existing obstacle sets?


	const JsonNode & biome = json["biome"];
	auto objectType = ObstacleSet::typeFromString(biome["objectType"].String());

	for (const JsonNode & type : data["types"])
	{
		for (const JsonNode & obstacle : type["templates"])
		{
			// TODO: Reuse templates (pointers) parsed by CObjectClassesHandler
		}
	}
}
*/

VCMI_LIB_NAMESPACE_END

