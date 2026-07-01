/*
 * SpellSchoolHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "SpellSchoolHandler.h"

#include "../GameLibrary.h"
#include "../json/JsonNode.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/TextIdentifier.h"

VCMI_LIB_NAMESPACE_BEGIN

std::string spells::SpellSchoolType::getNameTextID() const
{
	return TextIdentifier( "spellSchool", modScope, identifier, "name" ).get();
}

std::string spells::SpellSchoolType::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

std::vector<JsonNode> SpellSchoolHandler::loadLegacyData()
{
	objects.resize(4);

	return std::vector<JsonNode>(4, JsonNode(JsonMap()));
}

std::shared_ptr<spells::SpellSchoolType> SpellSchoolHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	auto ret = std::make_shared<spells::SpellSchoolType>();

	ret->id = SpellSchool(index);
	ret->modScope = scope;
	ret->identifier = identifier;
	ret->spellBordersPath = AnimationPath::fromJson(json["schoolBorders"]);
	ret->schoolBookmarkPath = AnimationPath::fromJson(json["schoolBookmark"]);
	ret->schoolHeaderPath = ImagePath::fromJson(json["schoolHeader"]);

	LIBRARY->generaltexth->registerString(scope, ret->getNameTextID(), json["name"]);

	return ret;
}

const std::vector<std::string> & SpellSchoolHandler::getTypeNames() const
{
	static const std::vector<std::string> names = {"spellSchool"};
	return names;
}

std::vector<SpellSchool> SpellSchoolHandler::getAllObjects() const
{
	std::vector<SpellSchool> result;

	for(const auto & school : objects)
		result.push_back(school->id);

	return result;
}

VCMI_LIB_NAMESPACE_END
