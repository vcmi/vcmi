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

std::string Resource::getNameTextID() const
{
	if(id.getNum() < GameConstants::RESOURCE_QUANTITY) // OH3 resources
		return TextIdentifier("core.restypes", id).get();
	return TextIdentifier( "resources", modScope, identifier, "name" ).get();
}

std::string Resource::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

void Resource::registerIcons(const IconRegistar & cb) const
{
	cb(getIconIndex(), 0, "SMALRES", iconSmall);
	cb(getIconIndex(), 0, "RESOURCE", iconMedium);
	cb(getIconIndex(), 0, "RESOUR82", iconLarge);
}

std::vector<JsonNode> ResourceTypeHandler::loadLegacyData()
{
	objects.resize(GameConstants::RESOURCE_QUANTITY);

	return std::vector<JsonNode>(GameConstants::RESOURCE_QUANTITY, JsonNode(JsonMap()));
}

std::shared_ptr<Resource> ResourceTypeHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	auto ret = std::make_shared<Resource>();

	ret->id = GameResID(index);
	ret->modScope = scope;
	ret->identifier = identifier;

	ret->price = json["price"].Integer();
	ret->iconSmall = json["images"]["small"].String();
	ret->iconMedium = json["images"]["medium"].String();
	ret->iconLarge = json["images"]["large"].String();

	if(ret->id.getNum() >= GameConstants::RESOURCE_QUANTITY) // not OH3 resources
		LIBRARY->generaltexth->registerString(scope, ret->getNameTextID(), json["name"]);

	return ret;
}

const std::vector<std::string> & ResourceTypeHandler::getTypeNames() const
{
	static const std::vector<std::string> types = { "resource" };
	return types;
}

void ResourceTypeHandler::afterLoadFinalization()
{
	for (const auto & resource : objects)
		if(resource)
			allObjects.push_back(resource->getId());
}

const std::vector<GameResID> & ResourceTypeHandler::getAllObjects() const
{
	return allObjects;
}

VCMI_LIB_NAMESPACE_END
