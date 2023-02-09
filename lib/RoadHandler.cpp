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
#include "CModHandler.h"
#include "CGeneralTextHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

RoadTypeHandler::RoadTypeHandler()
{
	objects.push_back(new RoadType);

	VLC->generaltexth->registerString("core", objects[0]->getNameTextID(), "");
}

RoadType * RoadTypeHandler::loadFromJson(
	const std::string & scope,
	const JsonNode & json,
	const std::string & identifier,
	size_t index)
{
	assert(identifier.find(':') == std::string::npos);

	RoadType * info = new RoadType;

	info->id              = RoadId(index);
	if (identifier.find(':') == std::string::npos)
		info->identifier = scope + ":" + identifier;
	else
		info->identifier = identifier;

	info->tilesFilename   = json["tilesFilename"].String();
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

std::vector<JsonNode> RoadTypeHandler::loadLegacyData(size_t dataSize)
{
	objects.resize(dataSize);
	return {};
}

std::vector<bool> RoadTypeHandler::getDefaultAllowed() const
{
	return {};
}

std::string RoadType::getNameTextID() const
{
	return TextIdentifier( "road", identifier,  "name" ).get();
}

std::string RoadType::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
}

RoadType::RoadType():
	id(Road::NO_ROAD),
	identifier("empty"),
	movementCost(GameConstants::BASE_MOVEMENT_COST)
{}
VCMI_LIB_NAMESPACE_END
