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
#include "IGameSettings.h"
#include "json/JsonNode.h"
#include "modding/IdentifierStorage.h"
#include "texts/CGeneralTextHandler.h"
#include "texts/CLegacyConfigParser.h"
#include "GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

std::shared_ptr<TerrainType> TerrainTypeHandler::loadFromJson( const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);

	auto info = std::make_shared<TerrainType>();

	info->id = TerrainId(index);
	info->identifier = identifier;
	info->modScope = scope;
	info->moveCost = static_cast<int>(json["moveCost"].Integer());
	if (json["music"].isVector())
	{
		for (auto const & entry : json["music"].Vector())
			info->musicFilename.push_back(AudioPath::fromJson(entry));
	}
	else
	{
		info->musicFilename.push_back(AudioPath::fromJson(json["music"]));
	}

	info->tilesFilename = AnimationPath::fromJson(json["tiles"]);
	info->horseSound = AudioPath::fromJson(json["horseSound"]);
	info->horseSoundPenalty = AudioPath::fromJson(json["horseSoundPenalty"]);
	info->transitionRequired = json["transitionRequired"].Bool();
	info->terrainViewPatterns = json["terrainViewPatterns"].String();

	LIBRARY->generaltexth->registerString(scope, info->getNameTextID(), json["text"]);

	const JsonVector & unblockedVec = json["minimapUnblocked"].Vector();
	info->minimapUnblocked =
	{
		static_cast<ui8>(unblockedVec[0].Float()),
		static_cast<ui8>(unblockedVec[1].Float()),
		static_cast<ui8>(unblockedVec[2].Float())
	};

	const JsonVector &blockedVec = json["minimapBlocked"].Vector();
	info->minimapBlocked =
	{
		static_cast<ui8>(blockedVec[0].Float()),
		static_cast<ui8>(blockedVec[1].Float()),
		static_cast<ui8>(blockedVec[2].Float())
	};

	info->passabilityType = 0;

	for(const auto& node : json["type"].Vector())
	{
		//Set bits
		const auto & s = node.String();
		if (s == "WATER") info->passabilityType |= TerrainType::PassabilityType::WATER;
		if (s == "ROCK") info->passabilityType |= TerrainType::PassabilityType::ROCK;
		if (s == "SURFACE") info->passabilityType |= TerrainType::PassabilityType::SURFACE;
		if (s == "SUB") info->passabilityType |= TerrainType::PassabilityType::SUBTERRANEAN;
	}

	info->river = River::NO_RIVER;
	if(!json["river"].isNull())
	{
		LIBRARY->identifiers()->requestIdentifier("river", json["river"], [info](int32_t identifier)
		{
			info->river = RiverId(identifier);
		});
	}

	for(const auto & t : json["paletteAnimation"].Vector())
	{
		TerrainPaletteAnimation element{
			static_cast<int>(t["start"].Integer()),
			static_cast<int>(t["length"].Integer())
		};
		info->paletteAnimation.push_back(element);
	}

	info->shortIdentifier = json["shortIdentifier"].String();
	assert(info->shortIdentifier.length() == 2);

	for(const auto & t : json["battleFields"].Vector())
	{
		LIBRARY->identifiers()->requestIdentifier("battlefield", t, [info](int32_t identifier)
		{
			info->battleFields.emplace_back(identifier);
		});
	}

	for(const auto & t : json["prohibitTransitions"].Vector())
	{
		LIBRARY->identifiers()->requestIdentifier("terrain", t, [info](int32_t identifier)
		{
			info->prohibitTransitions.emplace_back(identifier);
		});
	}

	info->rockTerrain = ETerrainId::ROCK;

	if(!json["rockTerrain"].isNull())
	{
		LIBRARY->identifiers()->requestIdentifier("terrain", json["rockTerrain"], [info](int32_t identifier)
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

std::vector<JsonNode> TerrainTypeHandler::loadLegacyData()
{
	size_t dataSize = LIBRARY->engineSettings()->getInteger(EGameSettings::TEXTS_TERRAIN);

	objects.resize(dataSize);

	CLegacyConfigParser terrainParser(TextPath::builtin("DATA/TERRNAME.TXT"));

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

bool TerrainType::isTransitionRequired() const
{
	return transitionRequired;
}

std::string TerrainType::getJsonKey() const
{
	return modScope + ":" + identifier;
}

std::string TerrainType::getModScope() const
{
	return modScope;
}

std::string TerrainType::getNameTextID() const
{
	return TextIdentifier( "terrain", modScope, identifier, "name" ).get();
}

std::string TerrainType::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

VCMI_LIB_NAMESPACE_END
