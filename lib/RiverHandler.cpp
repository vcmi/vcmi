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
#include "RiverHandler.h"
#include "CModHandler.h"
#include "CGeneralTextHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

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
	assert(identifier.find(':') == std::string::npos);

	RiverType * info = new RiverType;

	info->id              = RiverId(index);
	if (identifier.find(':') == std::string::npos)
		info->identifier = scope + ":" + identifier;
	else
		info->identifier = identifier;

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
	identifier("core:empty")
{}

VCMI_LIB_NAMESPACE_END
