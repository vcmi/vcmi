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
		LAKES, // Including dry or lava lakes
		CRATERS, // Chasms, Canyons, etc.
		ROCKS,
		PLANTS, // Flowers, cacti, mushrooms, logs, shrubs, etc.
		STRUCTURES, // Buildings, ruins, etc.
		ANIMALS, // Living, or bones
		OTHER // Crystals, shipwrecks, barrels, etc.
	};

	ObstacleSet();
	explicit ObstacleSet(EObstacleType type, TerrainId terrain);

	void addObstacle(std::shared_ptr<const ObjectTemplate> obstacle);
	void removeEmptyTemplates();
	std::vector<std::shared_ptr<const ObjectTemplate>> getObstacles() const;

	EObstacleType getType() const;
	void setType(EObstacleType type);

	std::set<TerrainId> getTerrains() const;
	void setTerrain(TerrainId terrain);
	void setTerrains(const std::set<TerrainId> & terrains);
	void addTerrain(TerrainId terrain);
	EMapLevel getLevel() const;
	void setLevel(EMapLevel level);
	std::set<EAlignment> getAlignments() const;
	void addAlignment(EAlignment alignment);
	std::set<FactionID> getFactions() const;
	void addFaction(FactionID faction);

	static EObstacleType typeFromString(const std::string &str);
	std::string toString() const;
	static EMapLevel levelFromString(const std::string &str);

	si32 id;

private:

	EObstacleType type;
	EMapLevel level;
	std::set<TerrainId> allowedTerrains; // Empty means all terrains
	std::set<FactionID> allowedFactions; // Empty means all factions
	std::set<EAlignment> allowedAlignments; // Empty means all alignments
	std::vector<std::shared_ptr<const ObjectTemplate>> obstacles;
};

using TObstacleTypes = std::vector<std::shared_ptr<ObstacleSet>>;

class DLL_LINKAGE ObstacleSetFilter
{
public:
	ObstacleSetFilter(ObstacleSet::EObstacleType allowedType, TerrainId terrain, EMapLevel level, FactionID faction, EAlignment alignment);
	ObstacleSetFilter(std::vector<ObstacleSet::EObstacleType> allowedTypes, TerrainId terrain, EMapLevel level, FactionID faction, EAlignment alignment);

	bool filter(const ObstacleSet &set) const;

	void setType(ObstacleSet::EObstacleType type);
	void setTypes(const std::vector<ObstacleSet::EObstacleType> & types);
	std::vector<ObstacleSet::EObstacleType> getAllowedTypes() const;
	TerrainId getTerrain() const;

	void setAlignment(EAlignment alignment);

private:
	std::vector<ObstacleSet::EObstacleType> allowedTypes;
	FactionID faction;
	EAlignment alignment;
// TODO: Filter by faction,  surface/underground, etc.
	const TerrainId terrain;
	EMapLevel level;
};

// TODO: Instantiate ObstacleSetHandler
class DLL_LINKAGE ObstacleSetHandler : public IHandlerBase
{
public:

	ObstacleSetHandler() = default;
	~ObstacleSetHandler() = default;

	std::vector<JsonNode> loadLegacyData() override;
	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;
	std::shared_ptr<ObstacleSet> loadFromJson(const std::string & scope, const JsonNode & json, const std::string & name, size_t index);

	ObstacleSet::EObstacleType convertObstacleClass(MapObjectID id);

	// TODO: Populate obstacleSets with all the obstacle sets from the game data
	void addTemplate(const std::string & scope, const std::string &name, std::shared_ptr<const ObjectTemplate> tmpl);

	void addObstacleSet(std::shared_ptr<ObstacleSet> set);

	void afterLoadFinalization() override;
	
	TObstacleTypes getObstacles(const ObstacleSetFilter &filter) const;

private:

	std::vector< std::shared_ptr<ObstacleSet> > biomes;

	// TODO: Serialize?
	std::map<si32, std::shared_ptr<const ObjectTemplate>> obstacleTemplates;

		// FIXME: Store pointers?
		std::map<ObstacleSet::EObstacleType, std::vector<std::shared_ptr<ObstacleSet>>> obstacleSets;
};

VCMI_LIB_NAMESPACE_END
