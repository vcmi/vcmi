/*
 * RumorTypeHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "RumorTypeHandler.h"

#include "../GameLibrary.h"
#include "../modding/LibraryLoadContext.h"
#include "../json/JsonNode.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/TextIdentifier.h"

std::string RumorType::getJsonKey() const
{
	return modScope + ":" + identifier;
}

std::string RumorType::getModScope() const
{
	return modScope;
}

std::string RumorType::getNameTextID() const
{
	return textID;
}

std::string RumorType::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(textID);
}

std::shared_ptr<RumorType> RumorTypeHandler::loadFromJson(LibraryLoadContext & context, const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);

	auto info = std::make_shared<RumorType>();

	info->id = RumorId(index);
	info->identifier = identifier;
	info->modScope = scope;

	const std::string & text = json["text"].String();
	if (!text.empty() && text[0] == '@')
	{
		info->textID = text.substr(1);
	}
	else
	{
		info->textID = TextIdentifier("rumor", scope, identifier, "text").get();
		context.lib.generaltexth->registerString(scope, info->textID, json["text"]);
	}

	if (!json["weight"].isNull())
		info->weight = json["weight"].Integer();

	context.resolver.requestIdentifierIfNotNull("faction", json["faction"], [info](int32_t factionIndex)
	{
		info->faction = FactionID(factionIndex);
	});

	return info;
}

void RumorTypeHandler::afterLoadFinalization(LibraryLoadContext & context)
{
	for (const auto & rumor : objects)
	{
		if (!context.lib.generaltexth->identifierExists(rumor->textID))
		{
			logMod->error("Mod '%s', rumor '%s', field 'text': text '%s' does not exist. Rumor will not be used", rumor->modScope, rumor->identifier, rumor->textID);
			continue;
		}
		allObjects.push_back(rumor->getId());
	}
}

const std::vector<std::string> & RumorTypeHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "rumor" };
	return typeNames;
}

std::vector<JsonNode> RumorTypeHandler::loadLegacyData(LibraryLoadContext & context)
{
	return {};
}

const std::vector<RumorId> & RumorTypeHandler::getAllObjects() const
{
	return allObjects;
}
