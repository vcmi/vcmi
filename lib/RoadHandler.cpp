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
#include "RoadHandler.h"
#include "texts/CGeneralTextHandler.h"
#include "GameSettings.h"
#include "json/JsonNode.h"
#include "VCMI_Lib.h"

VCMI_LIB_NAMESPACE_BEGIN

RoadTypeHandler::RoadTypeHandler()
{
	objects.emplace_back(new RoadType());

	VLC->generaltexth->registerString("core", objects[0]->getNameTextID(), "");
}

std::shared_ptr<RoadType> RoadTypeHandler::loadFromJson(
	const std::string & scope,
	const JsonNode & json,
	const std::string & identifier,
	size_t index)
{
	assert(identifier.find(':') == std::string::npos);

	auto info = std::make_shared<RoadType>();

	info->id              = RoadId(index);
	info->identifier      = identifier;
	info->modScope        = scope;
	info->tilesFilename   = AnimationPath::fromJson(json["tilesFilename"]);
	info->shortIdentifier = json["shortIdentifier"].String();
	info->movementCost    = json["moveCost"].Integer();

	VLC->generaltexth->registerString(scope,info->getNameTextID(), json["text"].String());

	return info;
}

const std::vector<std::string> & RoadTypeHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "road" };
	return typeNames;
}

std::vector<JsonNode> RoadTypeHandler::loadLegacyData()
{
	size_t dataSize = VLC->settings()->getInteger(EGameSettings::TEXTS_ROAD);

	objects.resize(dataSize);
	return {};
}

std::string RoadType::getJsonKey() const
{
	return modScope + ":" + identifier;
}

std::string RoadType::getModScope() const
{
	return modScope;
}

std::string RoadType::getNameTextID() const
{
	return TextIdentifier( "road", modScope, identifier, "name" ).get();
}

std::string RoadType::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
}

RoadType::RoadType():
	id(Road::NO_ROAD),
	identifier("empty"),
	modScope("core"),
	movementCost(GameConstants::BASE_MOVEMENT_COST)
{}
VCMI_LIB_NAMESPACE_END
