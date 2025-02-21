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
#include "texts/CGeneralTextHandler.h"
#include "IGameSettings.h"
#include "json/JsonNode.h"
#include "GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

RiverTypeHandler::RiverTypeHandler()
{
	objects.emplace_back(new RiverType());

	LIBRARY->generaltexth->registerString("core", objects[0]->getNameTextID(), "");
}

std::shared_ptr<RiverType> RiverTypeHandler::loadFromJson(
	const std::string & scope,
	const JsonNode & json,
	const std::string & identifier,
	size_t index)
{
	assert(identifier.find(':') == std::string::npos);

	auto info = std::make_shared<RiverType>();

	info->id              = RiverId(index);
	info->identifier      = identifier;
	info->modScope        = scope;
	info->tilesFilename   = AnimationPath::fromJson(json["tilesFilename"]);
	info->shortIdentifier = json["shortIdentifier"].String();
	info->deltaName       = json["delta"].String();

	for(const auto & t : json["paletteAnimation"].Vector())
	{
		RiverPaletteAnimation element{
			static_cast<int>(t["start"].Integer()),
			static_cast<int>(t["length"].Integer())
		};
		info->paletteAnimation.push_back(element);
	}

	LIBRARY->generaltexth->registerString(scope, info->getNameTextID(), json["text"]);

	return info;
}

const std::vector<std::string> & RiverTypeHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "river" };
	return typeNames;
}

std::vector<JsonNode> RiverTypeHandler::loadLegacyData()
{
	size_t dataSize = LIBRARY->engineSettings()->getInteger(EGameSettings::TEXTS_RIVER);

	objects.resize(dataSize);
	return {};
}

std::string RiverType::getJsonKey() const
{
	return modScope + ":" + identifier;
}

std::string RiverType::getModScope() const
{
	return modScope;
}

std::string RiverType::getNameTextID() const
{
	return TextIdentifier( "river", modScope, identifier, "name" ).get();
}

std::string RiverType::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

RiverType::RiverType():
	id(River::NO_RIVER),
	identifier("empty"),
	modScope("core")
{}

VCMI_LIB_NAMESPACE_END
