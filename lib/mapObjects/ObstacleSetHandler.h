/*
 * ObstacleSetHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../constants/EntityIdentifiers.h"
#include "../IHandlerBase.h"
#include "../json/JsonNode.h"
#include "ObjectTemplate.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE ObstacleSet
{
public:

	// TODO: Create string constants for these

	enum EObstacleType
	{
		INVALID = -1,
		MOUNTAINS = 0,
		TREES,
		LAKES, // Inluding dry or lava lakes
		CRATERS, // Chasms, Canyons, etc.
		ROCKS,
		PLANTS, // Flowers, cacti, mushrooms, logs, shrubs, etc.
		STRUCTURES, // Buildings, ruins, etc.
		ANIMALS, // Living, or bones
		OTHER // Crystals, shipwrecks, barrels, etc.
	};
	explicit ObstacleSet(EObstacleType type, TerrainId terrain);

	void addObstacle(std::shared_ptr<const ObjectTemplate> obstacle);
	std::vector<std::shared_ptr<const ObjectTemplate>> getObstacles() const;

	EObstacleType getType() const;
	TerrainId getTerrain() const;

	static EObstacleType typeFromString(const std::string &str);

private:
	EObstacleType type;
	TerrainId terrain;
	std::vector<std::shared_ptr<const ObjectTemplate>> obstacles;
};

typedef std::vector<ObstacleSet> TObstacleTypes;

class DLL_LINKAGE ObstacleSetFilter
{
public:
	ObstacleSetFilter(ObstacleSet::EObstacleType allowedType, TerrainId terrain);
	ObstacleSetFilter(std::vector<ObstacleSet::EObstacleType> allowedTypes, TerrainId terrain);

	bool filter(const ObstacleSet &set) const;

	std::vector<ObstacleSet::EObstacleType> getAllowedTypes() const;
	TerrainId getTerrain() const;

private:
	std::vector<ObstacleSet::EObstacleType> allowedTypes;
// TODO: Filter by faction, alignment, surface/underground, etc.
	const TerrainId terrain;
};

// TODO: Instantiate ObstacleSetHandler
class DLL_LINKAGE ObstacleSetHandler : public IHandlerBase, boost::noncopyable
{
public:

	ObstacleSetHandler() = default;
	~ObstacleSetHandler() = default;

	// FIXME: Not needed at all
	std::vector<JsonNode> loadLegacyData() override {return {};};
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data) override {};
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override {};

	ObstacleSet::EObstacleType convertObstacleClass(MapObjectID id);

	// TODO: Populate obstacleSets with all the obstacle sets from the game data

	void addObstacleSet(const ObstacleSet &set);
	
	TObstacleTypes getObstacles(const ObstacleSetFilter &filter) const;

private:

	std::map<ObstacleSet::EObstacleType, std::vector<ObstacleSet>> obstacleSets;
};

VCMI_LIB_NAMESPACE_END