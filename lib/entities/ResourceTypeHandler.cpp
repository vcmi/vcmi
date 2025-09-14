/*
 * ResourceTypeHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ResourceTypeHandler.h"

#include "../GameLibrary.h"
#include "../json/JsonNode.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/TextIdentifier.h"

VCMI_LIB_NAMESPACE_BEGIN

std::string resources::ResourceType::getNameTextID() const
{
	return TextIdentifier( "resources", modScope, identifier, "name" ).get();
}

std::string resources::ResourceType::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

std::vector<JsonNode> ResourceTypeHandler::loadLegacyData()
{
	objects.resize(8);

	return std::vector<JsonNode>(8, JsonNode(JsonMap()));
}

std::shared_ptr<resources::ResourceType> ResourceTypeHandler::loadObjectImpl(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto ret = std::make_shared<resources::ResourceType>();

	ret->id = GameResID(index);
	ret->modScope = scope;
	ret->identifier = name;

	ret->price = data["price"].Integer();

	LIBRARY->generaltexth->registerString(scope, ret->getNameTextID(), data["name"]);

	return ret;
}

/// loads single object into game. Scope is namespace of this object, same as name of source mod
void ResourceTypeHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	objects.push_back(loadObjectImpl(scope, name, data, objects.size()));
	registerObject(scope, "resources", name, objects.back()->getIndex());
}

void ResourceTypeHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	assert(objects[index] == nullptr); // ensure that this id was not loaded before
	objects[index] = loadObjectImpl(scope, name, data, index);
	registerObject(scope, "resources", name, objects[index]->getIndex());
}

std::vector<GameResID> ResourceTypeHandler::getAllObjects() const
{
	std::vector<GameResID> result;

	for (const auto & resource : objects)
		result.push_back(resource->id);

	return result;
}

VCMI_LIB_NAMESPACE_END
