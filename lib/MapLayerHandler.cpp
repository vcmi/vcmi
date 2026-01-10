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

VCMI_LIB_NAMESPACE_BEGIN

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

	LIBRARY->generaltexth->registerString(scope, info->getNameTextID(), json["text"]);

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

MapLayerType::MapLayerType():
	id(MapLayerId::UNKNOWN),
	identifier("empty"),
	modScope("core")
{}
VCMI_LIB_NAMESPACE_END
