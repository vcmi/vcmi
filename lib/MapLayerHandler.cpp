/*
 * MapLayerHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapLayerHandler.h"
#include "texts/CGeneralTextHandler.h"
#include "IGameSettings.h"
#include "json/JsonNode.h"
#include "GameLibrary.h"
#include "modding/IdentifierStorage.h"

MapLayerTypeHandler::MapLayerTypeHandler()
{
	objects.resize(3);
}

std::shared_ptr<MapLayerType> MapLayerTypeHandler::loadFromJson(
	const std::string & scope,
	const JsonNode & json,
	const std::string & identifier,
	size_t index)
{
	assert(identifier.find(':') == std::string::npos);

	auto info = std::make_shared<MapLayerType>();

	info->id              = MapLayerId(index);
	info->identifier      = identifier;
	info->modScope        = scope;
	info->icon            = json["icon"].String();

	LIBRARY->generaltexth->registerString(scope, info->getNameTextID(), json["text"]);

	LIBRARY->identifiers()->requestIdentifierIfNotNull("terrain", json["defaultTerrain"], [info](int32_t terrainIndex)
	{
		info->defaultTerrain = TerrainId(terrainIndex);
	});

	return info;
}

const std::vector<std::string> & MapLayerTypeHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "mapLayer" };
	return typeNames;
}

std::vector<JsonNode> MapLayerTypeHandler::loadLegacyData()
{
	return {};
}

std::string MapLayerType::getJsonKey() const
{
	return modScope + ":" + identifier;
}

std::string MapLayerType::getModScope() const
{
	return modScope;
}

std::string MapLayerType::getNameTextID() const
{
	return TextIdentifier( "mapLayer", modScope, identifier, "name" ).get();
}

std::string MapLayerType::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

void MapLayerType::registerIcons(const IconRegistar & cb) const
{
	if (!icon.empty())
		cb(getIconIndex(), 0, "MAPLAYERS", icon);
}

MapLayerType::MapLayerType():
	identifier("empty"),
	modScope("core"),
	id(MapLayerId::UNKNOWN),
	defaultTerrain(ETerrainId::WATER)
{}
