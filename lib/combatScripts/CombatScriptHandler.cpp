/*
 * CombatScriptHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CombatScriptHandler.h"

#include "ICombatEventScript.h"
#include "../GameLibrary.h"
#include "../json/JsonNode.h"
#include "../texts/CGeneralTextHandler.h"

std::shared_ptr<ICombatEventScript> CombatScriptHandler::get(CombatScriptID scriptID) const
{
	const auto & script = scriptTypes.at(scriptID.getNum());
	return scriptTypeFactories.at(script.type)->get(script.scriptId);
}

std::string CombatScriptHandler::getDescriptionTextID(CombatScriptID scriptID) const
{
	return scriptTypes.at(scriptID.getNum()).descriptionTextID;
}

void CombatScriptHandler::registerFactory(const std::string & typeName, std::shared_ptr<ICombatScriptFactory> factory)
{
	scriptTypeFactories[typeName] = factory;
}

std::vector<JsonNode> CombatScriptHandler::loadLegacyData()
{
	return {};
}

void CombatScriptHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	CombatScriptType newScript;
	newScript.identifier = name;
	newScript.modScope = scope;
	newScript.type = data["type"].String();
	newScript.scriptName = data["script"].String();
	newScript.descriptionTextID = TextIdentifier(scope, "combatScript", name, "description").get();

	LIBRARY->generaltexth->registerString(scope, newScript.descriptionTextID, data["description"]);

	for(const auto & patchEntry : data["patches"].Vector())
		newScript.patches.emplace_back(patchEntry.getModScope(), patchEntry.String());

	newScript.scriptId = scope + ':' + name;
	registerObject(scope, "combatScript", name, data, scriptTypes.size());
	scriptTypes.push_back(std::move(newScript));

	const auto & back = scriptTypes.back();
	scriptTypeFactories.at(back.type)->initialize(back.scriptId, back.modScope, back.scriptName, back.patches);
}

void CombatScriptHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	throw std::runtime_error("Not supported");
}
