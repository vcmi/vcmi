/*
 * TerrainHandler.h, part of VCMI engine
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
#include "GameConstants.h"
#include "IHandlerBase.h"
#include "Color.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE TerrainType : public EntityT<TerrainId>
{
	friend class TerrainTypeHandler;
	std::string identifier;
	TerrainId id;
	ui8 passabilityType{};

public:
	int32_t getIndex() const override { return id.getNum(); }
	int32_t getIconIndex() const override { return 0; }
	std::string getJsonKey() const override { return identifier;}
	void registerIcons(const IconRegistar & cb) const override {}
	TerrainId getId() const override { return id;}
	void updateFrom(const JsonNode & data) {};

	std::string getNameTextID() const override;
	std::string getNameTranslated() const override;

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
	ColorRGBA minimapBlocked;
	ColorRGBA minimapUnblocked;
	std::string shortIdentifier;
	std::string musicFilename;
	std::string tilesFilename;
	std::string terrainViewPatterns;
	std::string horseSound;
	std::string horseSoundPenalty;

	TerrainId rockTerrain;
	RiverId river;
	int moveCost{};
	bool transitionRequired{};
	
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

class DLL_LINKAGE TerrainTypeService : public EntityServiceT<TerrainId, TerrainType>
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

VCMI_LIB_NAMESPACE_END
