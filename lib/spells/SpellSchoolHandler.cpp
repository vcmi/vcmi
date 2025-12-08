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

std::shared_ptr<spells::SpellSchoolType> SpellSchoolHandler::loadObjectImpl(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto ret = std::make_shared<spells::SpellSchoolType>();

	ret->id = SpellSchool(index);
	ret->modScope = scope;
	ret->identifier = name;
	ret->spellBordersPath = AnimationPath::fromJson(data["schoolBorders"]);
	ret->schoolBookmarkPath = AnimationPath::fromJson(data["schoolBookmark"]);
	ret->schoolHeaderPath = ImagePath::fromJson(data["schoolHeader"]);

	LIBRARY->generaltexth->registerString(scope, ret->getNameTextID(), data["name"]);

	return ret;
}

/// loads single object into game. Scope is namespace of this object, same as name of source mod
void SpellSchoolHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	objects.push_back(loadObjectImpl(scope, name, data, objects.size()));
	registerObject(scope, "spellSchool", name, objects.back()->getIndex());
}

void SpellSchoolHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	assert(objects[index] == nullptr); // ensure that this id was not loaded before
	objects[index] = loadObjectImpl(scope, name, data, index);
	registerObject(scope, "spellSchool", name, objects[index]->getIndex());
}

std::vector<SpellSchool> SpellSchoolHandler::getAllObjects() const
{
	std::vector<SpellSchool> result;

	for (const auto & school : objects)
		result.push_back(school->id);

	return result;
}

VCMI_LIB_NAMESPACE_END
