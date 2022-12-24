/*
 * Terrain.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/EntityService.h>
#include <vcmi/Entity.h>
#include "ConstTransitivePtr.h"
#include "GameConstants.h"
#include "JsonNode.h"
#include "IHandlerBase.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE TerrainType : public EntityT<TerrainId>
{
public:
	int32_t getIndex() const override { return id.getNum(); }
	int32_t getIconIndex() const override { return 0; }
	const std::string & getName() const override { return identifier;}
	const std::string & getJsonKey() const override { return identifier;}
	void registerIcons(const IconRegistar & cb) const override {}
	TerrainId getId() const override { return id;}

	enum PassabilityType : ui8
	{
		LAND = 1,
		WATER = 2,
		SURFACE = 4,
		SUBTERRANEAN = 8,
		ROCK = 16
	};
	
	std::vector<BattleField> battleFields;
	std::vector<TerrainId> prohibitTransitions;
	std::array<int, 3> minimapBlocked;
	std::array<int, 3> minimapUnblocked;
	std::string identifier;
	std::string shortIdentifier;
	std::string musicFilename;
	std::string tilesFilename;
	std::string nameTranslated;
	std::string terrainViewPatterns;
	std::string horseSound;
	std::string horseSoundPenalty;

	TerrainId id;
	TerrainId rockTerrain;
	RiverId river;
	int moveCost;
	ui8 passabilityType;
	bool transitionRequired;
	
	TerrainType();
	
	bool isLand() const;
	bool isWater() const;
	bool isPassable() const;
	bool isSurface() const;
	bool isUnderground() const;
	bool isTransitionRequired() const;
	bool isSurfaceCartographerCompatible() const;
	bool isUndergroundCartographerCompatible() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & battleFields;
		h & prohibitTransitions;
		h & minimapBlocked;
		h & minimapUnblocked;
		h & identifier;
		h & musicFilename;
		h & tilesFilename;
		h & nameTranslated;
		h & shortIdentifier;
		h & terrainViewPatterns;
		h & rockTerrain;
		h & river;

		h & id;
		h & moveCost;
		h & horseSound;
		h & horseSoundPenalty;
		h & passabilityType;
		h & transitionRequired;
	}
};

class DLL_LINKAGE RiverType : public EntityT<RiverId>
{
public:
	int32_t getIndex() const override { return id.getNum(); }
	int32_t getIconIndex() const override { return 0; }
	const std::string & getName() const override { return identifier;}
	const std::string & getJsonKey() const override { return identifier;}
	void registerIcons(const IconRegistar & cb) const override {}
	RiverId getId() const override { return id;}

	std::string tilesFilename;
	std::string identifier;
	std::string shortIdentifier;
	std::string deltaName;
	RiverId id;

	RiverType();

	template <typename Handler> void serialize(Handler& h, const int version)
	{
		h & tilesFilename;
		h & identifier;
		h & deltaName;
		h & id;
	}
};

class DLL_LINKAGE RoadType : public EntityT<RoadId>
{
public:
	int32_t getIndex() const override { return id.getNum(); }
	int32_t getIconIndex() const override { return 0; }
	const std::string & getName() const override { return identifier;}
	const std::string & getJsonKey() const override { return identifier;}
	void registerIcons(const IconRegistar & cb) const override {}
	RoadId getId() const override { return id;}

	std::string tilesFilename;
	std::string identifier;
	std::string shortIdentifier;
	RoadId id;
	ui8 movementCost;

	RoadType();

	template <typename Handler> void serialize(Handler& h, const int version)
	{
		h & tilesFilename;
		h & identifier;
		h & id;
		h & movementCost;
	}
};

class DLL_LINKAGE TerrainTypeService : public EntityServiceT<TerrainId, TerrainType>
{
public:
};

class DLL_LINKAGE RiverTypeService : public EntityServiceT<RiverId, RiverType>
{
public:
};

class DLL_LINKAGE RoadTypeService : public EntityServiceT<RoadId, RoadType>
{
public:
};

class DLL_LINKAGE TerrainTypeHandler : public CHandlerBase<TerrainId, TerrainType, TerrainType, TerrainTypeService>
{
public:
	virtual TerrainType * loadFromJson(
		const std::string & scope,
		const JsonNode & json,
		const std::string & identifier,
		size_t index) override;

	virtual const std::vector<std::string> & getTypeNames() const override;
	virtual std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
	virtual std::vector<bool> getDefaultAllowed() const override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
	}
};

class DLL_LINKAGE RiverTypeHandler : public CHandlerBase<RiverId, RiverType, RiverType, RiverTypeService>
{
public:
	virtual RiverType * loadFromJson(
		const std::string & scope,
		const JsonNode & json,
		const std::string & identifier,
		size_t index) override;

	RiverTypeHandler();

	virtual const std::vector<std::string> & getTypeNames() const override;
	virtual std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
	virtual std::vector<bool> getDefaultAllowed() const override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
	}
};

class DLL_LINKAGE RoadTypeHandler : public CHandlerBase<RoadId, RoadType, RoadType, RoadTypeService>
{
public:
	virtual RoadType * loadFromJson(
		const std::string & scope,
		const JsonNode & json,
		const std::string & identifier,
		size_t index) override;

	RoadTypeHandler();

	virtual const std::vector<std::string> & getTypeNames() const override;
	virtual std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
	virtual std::vector<bool> getDefaultAllowed() const override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
	}
};

VCMI_LIB_NAMESPACE_END
