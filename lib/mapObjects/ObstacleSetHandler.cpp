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
#include "../constants/StringConstants.h"
#include "../TerrainHandler.h"
#include "../GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

ObstacleSet::ObstacleSet():
	type(INVALID),
	level(EMapLevel::ANY),
	allowedTerrains({TerrainId::NONE})
{
}

ObstacleSet::ObstacleSet(EObstacleType type, TerrainId terrain):
	type(type),
	level(EMapLevel::ANY),
	allowedTerrains({terrain})
{
}

void ObstacleSet::addObstacle(std::shared_ptr<const ObjectTemplate> obstacle)
{
	obstacles.push_back(obstacle);
}

void ObstacleSet::removeEmptyTemplates()
{
	vstd::erase_if(obstacles, [](const std::shared_ptr<const ObjectTemplate> &tmpl)
	{
		if (tmpl->getBlockedOffsets().empty())
		{
			logMod->debug("Obstacle template %s blocks no tiles, removing it", tmpl->stringID);
			return true;
		}
		return false;
	});
}

ObstacleSetFilter::ObstacleSetFilter(std::vector<ObstacleSet::EObstacleType> allowedTypes,
	TerrainId terrain = TerrainId::ANY_TERRAIN,
	EMapLevel level = EMapLevel::ANY,
	FactionID faction = FactionID::ANY,
	EAlignment alignment = EAlignment::ANY):
	allowedTypes(allowedTypes),
	faction(faction),
	alignment(alignment),
	terrain(terrain),
	level(level)
{
}

ObstacleSetFilter::ObstacleSetFilter(ObstacleSet::EObstacleType allowedType,
	TerrainId terrain = TerrainId::ANY_TERRAIN,
	EMapLevel level = EMapLevel::ANY,
	FactionID faction = FactionID::ANY,
	EAlignment alignment = EAlignment::ANY):
	allowedTypes({allowedType}),
	faction(faction),
	alignment(alignment),
	terrain(terrain),
	level(level)
{
}

bool ObstacleSetFilter::filter(const ObstacleSet &set) const
{
	if (terrain != TerrainId::ANY_TERRAIN && !vstd::contains(set.getTerrains(), terrain))
	{
		return false;
	}

	if (level != EMapLevel::ANY && set.getLevel() != EMapLevel::ANY)
	{
		if (level != set.getLevel())
		{
			return false;
		}
	}

	if (faction != FactionID::ANY)
	{
		auto factions = set.getFactions();
		if (!factions.empty() && !vstd::contains(factions, faction))
		{
			return false;
		}
	}

	// TODO: Also check specific factions
	if (alignment != EAlignment::ANY)
	{
		auto alignments = set.getAlignments();
		if (!alignments.empty() && !vstd::contains(alignments, alignment))
		{
			return false;
		}
	}

	return true;
}

TerrainId ObstacleSetFilter::getTerrain() const
{
	return terrain;
}

std::set<TerrainId> ObstacleSet::getTerrains() const
{
	return allowedTerrains;
}

void ObstacleSet::setTerrain(TerrainId terrain)
{
	this->allowedTerrains = {terrain};
}

void ObstacleSet::setTerrains(const std::set<TerrainId> & terrains)
{
	this->allowedTerrains = terrains;
}

void ObstacleSet::addTerrain(TerrainId terrain)
{
	this->allowedTerrains.insert(terrain);
}

EMapLevel ObstacleSet::getLevel() const
{
	return level;
}

void ObstacleSet::setLevel(EMapLevel newLevel)
{
	level = newLevel;
}

std::set<FactionID> ObstacleSet::getFactions() const
{
	return allowedFactions;
}

void ObstacleSet::addFaction(FactionID faction)
{
	this->allowedFactions.insert(faction);
}

void ObstacleSet::addAlignment(EAlignment alignment)
{
	this->allowedAlignments.insert(alignment);
}

std::set<EAlignment> ObstacleSet::getAlignments() const
{
	return allowedAlignments;
}

ObstacleSet::EObstacleType ObstacleSet::getType() const
{
	return type;
}

void ObstacleSet::setType(EObstacleType newType)
{
	type = newType;
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

EMapLevel ObstacleSet::levelFromString(const std::string &str)
{
	static const std::map<std::string, EMapLevel> LEVEL_NAMES =
	{
		{"surface", EMapLevel::SURFACE},
		{"underground", EMapLevel::UNDERGROUND}
	};

	if (LEVEL_NAMES.find(str) != LEVEL_NAMES.end())
	{
		return LEVEL_NAMES.at(str);
	}

	throw std::runtime_error("Invalid map level: " + str);
}

std::vector<ObstacleSet::EObstacleType> ObstacleSetFilter::getAllowedTypes() const
{
	return allowedTypes;
}

void ObstacleSetFilter::setType(ObstacleSet::EObstacleType type)
{
	allowedTypes = {type};
}

void ObstacleSetFilter::setTypes(const std::vector<ObstacleSet::EObstacleType> & types)
{
	this->allowedTypes = types;
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
		// TODO: Define some const array of object types ("biome" etc.)
		LIBRARY->identifiersHandler->registerObject(scope, "biome", name, biomes.back()->id);
	}
	else
	{
		logMod->error("Failed to load obstacle set: %s", name);
	}
}

void ObstacleSetHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	//Unused
	loadObject(scope, name, data);
}

std::shared_ptr<ObstacleSet> ObstacleSetHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & name, size_t index)
{
	auto os = std::make_shared<ObstacleSet>();
	os->id = index;

	auto biome = json["biome"].Struct();
	os->setType(ObstacleSet::typeFromString(biome["objectType"].String()));

	// TODO: Handle any (every) terrain option

	if (biome["terrain"].isString())
	{
		auto terrainName = biome["terrain"].String();

		LIBRARY->identifiers()->requestIdentifier(scope, "terrain", terrainName, [os](si32 id)
		{
			os->setTerrain(TerrainId(id));
		});
	}
	else if (biome["terrain"].isVector())
	{
		auto terrains = biome["terrain"].Vector();

		for (const auto & terrain : terrains)
		{
			LIBRARY->identifiers()->requestIdentifier(scope, "terrain", terrain.String(), [os](si32 id)
			{
				os->addTerrain(TerrainId(id));
			});
		}
	}
	else
	{
		logMod->error("No terrain specified for obstacle set %s", name);
	}

	if (biome["level"].isString())
	{
		auto level = biome["level"].String();
		os->setLevel(ObstacleSet::levelFromString(level));
	}

	auto handleFaction = [os, scope](const std::string & str)
	{
		LIBRARY->identifiers()->requestIdentifier(scope, "faction", str, [os](si32 id)
		{
			os->addFaction(FactionID(id));
		});
	};

	if (biome["faction"].isString())
	{
		auto factionName = biome["faction"].String();
		handleFaction(factionName);
	}
	else if (biome["faction"].isVector())
	{
		auto factions = biome["faction"].Vector();
		for (const auto & node : factions)
		{
			handleFaction(node.String());
		}
	}

	// TODO: Move this parser to some utils
	auto parseAlignment = [](const std::string & str) ->EAlignment
	{
		int alignment = vstd::find_pos(GameConstants::ALIGNMENT_NAMES, str);
		if (alignment == -1)
		{
			logMod->error("Incorrect alignment: ", str);
			return EAlignment::ANY;
		}
		else
		{
			return static_cast<EAlignment>(alignment);
		}
	};

	if (biome["alignment"].isString())
	{
		os->addAlignment(parseAlignment(biome["alignment"].String()));
	}
	else if (biome["alignment"].isVector())
	{
		auto alignments = biome["alignment"].Vector();
		for (const auto & node : alignments)
		{
			os->addAlignment(parseAlignment(node.String()));
		}
	}

	auto templates = json["templates"].Vector();
	for (const auto & node : templates)
	{
		logMod->trace("Registering obstacle template: %s in scope %s", node.String(), scope);

		auto identifier = boost::algorithm::to_lower_copy(node.String());
		auto jsonName = JsonNode(identifier);

		LIBRARY->identifiers()->requestIdentifier(node.getModScope(), "obstacleTemplate", identifier, [this, os](si32 id)
		{
			logMod->trace("Resolved obstacle id: %d", id);
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

	if (LIBRARY->identifiersHandler->getIdentifier(scope, "obstacleTemplate", strippedName, true))
	{
		logMod->debug("Duplicate obstacle template: %s", strippedName);
		return;
	}
	else
	{
		// Save by name
		LIBRARY->identifiersHandler->registerObject(scope, "obstacleTemplate", strippedName, id);

		// Index by id
		obstacleTemplates[id] = tmpl;
	}
}

void ObstacleSetHandler::addObstacleSet(std::shared_ptr<ObstacleSet> os)
{
	biomes.push_back(os);
}

void ObstacleSetHandler::afterLoadFinalization()
{
	for(const auto & os : biomes)
		os->removeEmptyTemplates();

	vstd::erase_if(biomes, [](const std::shared_ptr<ObstacleSet> &os)
	{
		if (os->getObstacles().empty())
		{
			logMod->warn("Obstacle set %d is empty, removing it", os->id);
			return true;
		}
		return false;
	});

	// Populate map
	for(const auto & os : biomes)
		obstacleSets[os->getType()].push_back(os);
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

