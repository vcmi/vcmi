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
	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	const std::string & getName() const override;
	const std::string & getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override;
	TerrainId getId() const override;

	enum PassabilityType : ui8
	{
		LAND = 1,
		WATER = 2,
		SURFACE = 4,
		SUBTERRANEAN = 8,
		ROCK = 16
	};
	
	std::vector<std::string> battleFields;
	std::vector<TerrainId> prohibitTransitions;
	std::array<int, 3> minimapBlocked;
	std::array<int, 3> minimapUnblocked;
	std::string name;
	std::string musicFilename;
	std::string tilesFilename;
	std::string terrainText;
	std::string typeCode;
	std::string terrainViewPatterns;
	RiverId river;

	TerrainId id;
	TerrainId rockTerrain;
	int moveCost;
	int horseSoundId;
	ui8 passabilityType;
	bool transitionRequired;
	
	TerrainType(const std::string & name = "");
	
	bool operator==(const TerrainType & other);
	bool operator!=(const TerrainType & other);
	bool operator<(const TerrainType & other);
	
	bool isLand() const;
	bool isWater() const;
	bool isPassable() const;
	bool isSurface() const;
	bool isUnderground() const;
	bool isTransitionRequired() const;
	bool isSurfaceCartographerCompatible() const;
	bool isUndergroundCartographerCompatible() const;
		
	operator std::string() const;
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & battleFields;
		h & prohibitTransitions;
		h & minimapBlocked;
		h & minimapUnblocked;
		h & name;
		h & musicFilename;
		h & tilesFilename;
		h & terrainText;
		h & typeCode;
		h & terrainViewPatterns;
		h & rockTerrain;
		h & river;

		h & id;
		h & moveCost;
		h & horseSoundId;
		h & passabilityType;
		h & transitionRequired;
	}
};

class DLL_LINKAGE RiverType : public EntityT<TerrainId>
{
public:
	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	const std::string & getName() const override;
	const std::string & getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override;
	TerrainId getId() const override;

	std::string fileName;
	std::string code;
	std::string deltaName;
	RiverId id;

	RiverType(const std::string & fileName = "", const std::string & code = "", RiverId id = River::NO_RIVER);

	template <typename Handler> void serialize(Handler& h, const int version)
	{
		h & fileName;
		h & code;
		h & deltaName;
		h & id;
	}
};

class DLL_LINKAGE RoadType : public EntityT<TerrainId>
{
public:
	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	const std::string & getName() const override;
	const std::string & getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override;
	TerrainId getId() const override;

	std::string fileName;
	std::string code;
	RoadId id;
	ui8 movementCost;

	RoadType(const std::string & fileName = "", const std::string& code = "", RoadId id = Road::NO_ROAD);

	template <typename Handler> void serialize(Handler& h, const int version)
	{
		h & fileName;
		h & code;
		h & id;
		h & movementCost;
	}
};

class DLL_LINKAGE TerrainTypeService : public EntityServiceT<TerrainId, TerrainType>
{
public:
};

class DLL_LINKAGE RiverTypeService : public EntityServiceT<TerrainId, RiverType>
{
public:
};

class DLL_LINKAGE RoadTypeService : public EntityServiceT<TerrainId, RoadType>
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

	TerrainType * getInfoByCode(const std::string & identifier);
	TerrainType * getInfoByName(const std::string & identifier);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
	}
};

class DLL_LINKAGE RiverTypeHandler : public CHandlerBase<TerrainId, RiverType, RiverType, RiverTypeService>
{
public:
	virtual RiverType * loadFromJson(
		const std::string & scope,
		const JsonNode & json,
		const std::string & identifier,
		size_t index) override;

	virtual const std::vector<std::string> & getTypeNames() const override;
	virtual std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
	virtual std::vector<bool> getDefaultAllowed() const override;

	RiverType * getInfoByCode(const std::string & identifier);
	RiverType * getInfoByName(const std::string & identifier);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
	}
};

class DLL_LINKAGE RoadTypeHandler : public CHandlerBase<TerrainId, RoadType, RoadType, RoadTypeService>
{
public:
	virtual RoadType * loadFromJson(
		const std::string & scope,
		const JsonNode & json,
		const std::string & identifier,
		size_t index) override;

	virtual const std::vector<std::string> & getTypeNames() const override;
	virtual std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
	virtual std::vector<bool> getDefaultAllowed() const override;

	RoadType * getInfoByCode(const std::string & identifier);
	RoadType * getInfoByName(const std::string & identifier);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
	}
};

VCMI_LIB_NAMESPACE_END
