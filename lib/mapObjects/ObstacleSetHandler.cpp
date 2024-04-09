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

#include "../modding/IdentifierStorage.h"

VCMI_LIB_NAMESPACE_BEGIN

ObstacleSet::ObstacleSet():
	type(INVALID),
	terrain(TerrainId::NONE)
{
}

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

void ObstacleSet::setTerrain(TerrainId terrain)
{
	this->terrain = terrain;
}

ObstacleSet::EObstacleType ObstacleSet::getType() const
{
	return type;
}

void ObstacleSet::setType(EObstacleType type)
{
	this->type = type;
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

std::string ObstacleSet::toString() const
{
	static const std::map<EObstacleType, std::string> OBSTACLE_TYPE_STRINGS =
	{
		{MOUNTAINS, "mountain"},
		{TREES, "tree"},
		{LAKES, "lake"},
		{CRATERS, "crater"},
		{ROCKS, "rock"},
		{PLANTS, "plant"},
		{STRUCTURES, "structure"},
		{ANIMALS, "animal"},
		{OTHER, "other"}
	};

	return OBSTACLE_TYPE_STRINGS.at(type);
}

std::vector<ObstacleSet::EObstacleType> ObstacleSetFilter::getAllowedTypes() const
{
	return allowedTypes;
}

std::vector<JsonNode> ObstacleSetHandler::loadLegacyData()
{
	return {};
}

void ObstacleSetHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto os = loadFromJson(scope, data, name, biomes.size());
	if(os)
	{
		addObstacleSet(os);
	}
	else
	{
		logMod->error("Failed to load obstacle set: %s", name);
	}
	// TODO: Define some const array of object types ("biome" etc.)
	VLC->identifiersHandler->registerObject(scope, "biome", name, biomes.back()->id);
}

void ObstacleSetHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto os = loadFromJson(scope, data, name, index);
	if(os)
	{
		addObstacleSet(os);
	}
	else
	{
		logMod->error("Failed to load obstacle set: %s", name);
	}
	VLC->identifiersHandler->registerObject(scope, "biome", name, biomes.at(index)->id);
}

std::shared_ptr<ObstacleSet> ObstacleSetHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & name, size_t index)
{
	auto os = std::make_shared<ObstacleSet>();

	auto biome = json["biome"].Struct();
	os->setType(ObstacleSet::typeFromString(biome["objectType"].String()));

	auto terrainName = biome["terrain"].String();

	VLC->identifiers()->requestIdentifier(scope, "terrain", terrainName, [os](si32 id)
	{
		os->setTerrain(TerrainId(id));
	});

	auto templates = json["templates"].Vector();
	for (const auto & node : templates)
	{
		logGlobal->info("Registering obstacle template: %s in scope %s", node.String(), scope);

		auto identifier = boost::algorithm::to_lower_copy(node.String());
		auto jsonName = JsonNode(identifier);

		VLC->identifiers()->requestIdentifier(node.getModScope(), "obstacleTemplate", identifier, [this, os](si32 id)
		{
			logGlobal->info("Resolved obstacle id: %d", id);
			os->addObstacle(obstacleTemplates[id]);
		});
	}

	return os;
}

void ObstacleSetHandler::addTemplate(const std::string & scope, const std::string &name, std::shared_ptr<const ObjectTemplate> tmpl)
{
	auto id = obstacleTemplates.size();

	auto strippedName = boost::algorithm::to_lower_copy(name);
	auto pos = strippedName.find(".def");
	if(pos != std::string::npos)
		strippedName.erase(pos, 4);

	if (VLC->identifiersHandler->getIdentifier(scope, "obstacleTemplate", strippedName, true))
	{
		logMod->warn("Duplicate obstacle template: %s", strippedName);
		return;
	}
	else
	{
		// Save by name
		VLC->identifiersHandler->registerObject(scope, "obstacleTemplate", strippedName, id);

		// Index by id
		obstacleTemplates[id] = tmpl;
	}
}

void ObstacleSetHandler::addObstacleSet(std::shared_ptr<ObstacleSet> os)
{
	// TODO: Allow to refer to existing obstacle set by its id (name)
	obstacleSets[os->getType()].push_back(os);
	biomes.push_back(os);
}

TObstacleTypes ObstacleSetHandler::getObstacles( const ObstacleSetFilter &filter) const
{
	TObstacleTypes result;

	for (const auto &allowedType : filter.getAllowedTypes())
	{
		auto it = obstacleSets.find(allowedType);
		if(it != obstacleSets.end())
		{
			for (const auto &os : it->second)
			{
				if (filter.filter(*os))
				{
					result.push_back(os);
				}
			}
		}
	}
	return result;
}

VCMI_LIB_NAMESPACE_END

