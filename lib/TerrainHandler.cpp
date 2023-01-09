/*
 * Terrain.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TerrainHandler.h"
#include "VCMI_Lib.h"
#include "CModHandler.h"
#include "CGeneralTextHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

//regular expression to change id for string at config
//("allowedTerrain"\s*:\s*\[.*)9(.*\],\n)
//\1"rock"\2

TerrainType * TerrainTypeHandler::loadFromJson( const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	TerrainType * info = new TerrainType;

	info->id = TerrainId(index);
	info->identifier = identifier;
	info->modScope = scope;

	info->moveCost = static_cast<int>(json["moveCost"].Integer());
	info->musicFilename = json["music"].String();
	info->tilesFilename = json["tiles"].String();
	info->horseSound = json["horseSound"].String();
	info->horseSoundPenalty = json["horseSoundPenalty"].String();
	info->transitionRequired = json["transitionRequired"].Bool();
	info->terrainViewPatterns = json["terrainViewPatterns"].String();

	VLC->generaltexth->registerString(info->getNameTextID(), json["text"].String());

	const JsonVector & unblockedVec = json["minimapUnblocked"].Vector();
	info->minimapUnblocked =
	{
		ui8(unblockedVec[0].Float()),
		ui8(unblockedVec[1].Float()),
		ui8(unblockedVec[2].Float())
	};

	const JsonVector &blockedVec = json["minimapBlocked"].Vector();
	info->minimapBlocked =
	{
		ui8(blockedVec[0].Float()),
		ui8(blockedVec[1].Float()),
		ui8(blockedVec[2].Float())
	};

	info->passabilityType = 0;

	for(const auto& node : json["type"].Vector())
	{
		//Set bits
		auto s = node.String();
		if (s == "LAND") info->passabilityType |= TerrainType::PassabilityType::LAND;
		if (s == "WATER") info->passabilityType |= TerrainType::PassabilityType::WATER;
		if (s == "ROCK") info->passabilityType |= TerrainType::PassabilityType::ROCK;
		if (s == "SURFACE") info->passabilityType |= TerrainType::PassabilityType::SURFACE;
		if (s == "SUB") info->passabilityType |= TerrainType::PassabilityType::SUBTERRANEAN;
	}

	info->river = River::NO_RIVER;
	if(!json["river"].isNull())
	{
		VLC->modh->identifiers.requestIdentifier("river", json["river"], [info](int32_t identifier)
		{
			info->river = RiverId(identifier);
		});
	}

	info->shortIdentifier = json["shortIdentifier"].String();
	assert(info->shortIdentifier.length() == 2);

	for(auto & t : json["battleFields"].Vector())
	{
		VLC->modh->identifiers.requestIdentifier("battlefield", t, [info](int32_t identifier)
		{
			info->battleFields.emplace_back(identifier);
		});
	}

	for(auto & t : json["prohibitTransitions"].Vector())
	{
		VLC->modh->identifiers.requestIdentifier("terrain", t, [info](int32_t identifier)
		{
			info->prohibitTransitions.emplace_back(identifier);
		});
	}

	info->rockTerrain = ETerrainId::ROCK;

	if(!json["rockTerrain"].isNull())
	{
		VLC->modh->identifiers.requestIdentifier("terrain", json["rockTerrain"], [info](int32_t identifier)
		{
			info->rockTerrain = TerrainId(identifier);
		});
	}

	return info;
}

const std::vector<std::string> & TerrainTypeHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "terrain" };
	return typeNames;
}

std::vector<JsonNode> TerrainTypeHandler::loadLegacyData(size_t dataSize)
{
	objects.resize(dataSize);

	CLegacyConfigParser terrainParser("DATA/TERRNAME.TXT");

	std::vector<JsonNode> result;
	do
	{
		JsonNode terrain;
		terrain["text"].String() = terrainParser.readString();
		result.push_back(terrain);
	}
	while (terrainParser.endLine());

	return result;
}

std::vector<bool> TerrainTypeHandler::getDefaultAllowed() const
{
	return {};
}

RiverTypeHandler::RiverTypeHandler()
{
	objects.push_back(new RiverType);
}

RiverType * RiverTypeHandler::loadFromJson(
	const std::string & scope,
	const JsonNode & json,
	const std::string & identifier,
	size_t index)
{
	RiverType * info = new RiverType;

	info->id              = RiverId(index);
	info->identifier      = identifier;
	info->tilesFilename   = json["tilesFilename"].String();
	info->shortIdentifier = json["shortIdentifier"].String();
	info->deltaName       = json["delta"].String();

	VLC->generaltexth->registerString(info->getNameTextID(), json["text"].String());

	return info;
}

const std::vector<std::string> & RiverTypeHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "river" };
	return typeNames;
}

std::vector<JsonNode> RiverTypeHandler::loadLegacyData(size_t dataSize)
{
	objects.resize(dataSize);
	return {};
}

std::vector<bool> RiverTypeHandler::getDefaultAllowed() const
{
	return {};
}

RoadTypeHandler::RoadTypeHandler()
{
	objects.push_back(new RoadType);
}

RoadType * RoadTypeHandler::loadFromJson(
	const std::string & scope,
	const JsonNode & json,
	const std::string & identifier,
	size_t index)
{
	RoadType * info = new RoadType;

	info->id              = RoadId(index);
	info->identifier      = identifier;
	info->tilesFilename   = json["tilesFilename"].String();
	info->shortIdentifier = json["shortIdentifier"].String();
	info->movementCost    = json["moveCost"].Integer();

	VLC->generaltexth->registerString(info->getNameTextID(), json["text"].String());

	return info;
}

const std::vector<std::string> & RoadTypeHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "road" };
	return typeNames;
}

std::vector<JsonNode> RoadTypeHandler::loadLegacyData(size_t dataSize)
{
	objects.resize(dataSize);
	return {};
}

std::vector<bool> RoadTypeHandler::getDefaultAllowed() const
{
	return {};
}

bool TerrainType::isLand() const
{
	return !isWater();
}

bool TerrainType::isWater() const
{
	return passabilityType & PassabilityType::WATER;
}

bool TerrainType::isPassable() const
{
	return !(passabilityType & PassabilityType::ROCK);
}

bool TerrainType::isSurface() const
{
	return passabilityType & PassabilityType::SURFACE;
}

bool TerrainType::isUnderground() const
{
	return passabilityType & PassabilityType::SUBTERRANEAN;
}

bool TerrainType::isSurfaceCartographerCompatible() const
{
	return isSurface();
}

bool TerrainType::isUndergroundCartographerCompatible() const
{
	return isLand() && isPassable() && !isSurface();
}

bool TerrainType::isTransitionRequired() const
{
	return transitionRequired;
}

std::string TerrainType::getNameTextID() const
{
	return TextIdentifier( "terrain", modScope, identifier,  "name" ).get();
}

std::string TerrainType::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
}

TerrainType::TerrainType()
{}

std::string RoadType::getNameTextID() const
{
	return TextIdentifier( "road", identifier,  "name" ).get();
}

std::string RoadType::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
}

std::string RiverType::getNameTextID() const
{
	return TextIdentifier( "river", identifier,  "name" ).get();
}

std::string RiverType::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
}

RiverType::RiverType():
	id(River::NO_RIVER),
	identifier("empty")
{}

RoadType::RoadType():
	id(Road::NO_ROAD),
	identifier("empty"),
	movementCost(GameConstants::BASE_MOVEMENT_COST)
{}
VCMI_LIB_NAMESPACE_END
