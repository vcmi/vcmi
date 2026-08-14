/*
 * ScriptHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ScriptHandler.h"

#include "../GameLibrary.h"
#include "../json/JsonNode.h"
#include "../json/JsonUtils.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/TextIdentifier.h"

namespace
{

ScriptKind parseKind(const std::string & name)
{
	if(name == "spellEffect")
		return ScriptKind::SPELL_EFFECT;
	if(name == "combatEvent")
		return ScriptKind::COMBAT_EVENT;

	return ScriptKind::INVALID;
}

}

const ScriptHandler::LoadedScript * ScriptHandler::find(ScriptID scriptID) const
{
	if(!scriptID.hasValue())
		return nullptr;

	return &scripts.at(scriptID.getNum());
}

std::shared_ptr<ICombatEventScript> ScriptHandler::getCombatEventScript(ScriptID scriptID) const
{
	const auto * script = find(scriptID);

	return script ? script->combatEventScript : nullptr;
}

std::shared_ptr<spells::effects::Effect> ScriptHandler::createSpellEffect(ScriptID scriptID) const
{
	const auto * script = find(scriptID);

	if(!script || script->description.kind != ScriptKind::SPELL_EFFECT)
		return nullptr;

	return factory->createSpellEffect(script->description.scriptId);
}

std::string ScriptHandler::getJsonKey(ScriptID scriptID) const
{
	const auto * script = find(scriptID);

	return script ? script->description.scriptId : std::string();
}

std::string ScriptHandler::getDescriptionTextID(ScriptID scriptID) const
{
	const auto * script = find(scriptID);

	return script ? script->description.descriptionTextID : std::string();
}

int ScriptHandler::getPriority(ScriptID scriptID) const
{
	const auto * script = find(scriptID);

	return script ? script->description.priority : 0;
}

ScriptKind ScriptHandler::getKind(ScriptID scriptID) const
{
	const auto * script = find(scriptID);

	return script ? script->description.kind : ScriptKind::INVALID;
}

void ScriptHandler::registerFactory(std::shared_ptr<IScriptFactory> newFactory)
{
	factory = std::move(newFactory);
}

std::vector<JsonNode> ScriptHandler::loadLegacyData()
{
	return {};
}

void ScriptHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	ScriptTypeDescription description;
	description.identifier = name;
	description.modScope = scope;
	description.scriptId = scope + ':' + name;
	description.sourcePath = data["script"].String();
	description.kind = parseKind(data["implements"].String());
	description.parametersSchema = data["schema"];
	description.priority = data["priority"].Integer();

	for(const auto & patchEntry : data["patches"].Vector())
		description.patches.emplace_back(patchEntry.getModScope(), patchEntry.String());

	for(const auto & item : data["stringRegistrations"].Vector())
		description.stringRegistrations.push_back(item.String());

	if(description.kind == ScriptKind::INVALID)
	{
		logMod->error("Script '%s' implements unknown kind '%s'! Script will not be loaded.", description.scriptId, data["implements"].String());
		return;
	}

	if(!data["description"].isNull())
	{
		description.descriptionTextID = TextIdentifier(scope, "script", name, "description").get();
		LIBRARY->generaltexth->registerString(scope, description.descriptionTextID, data["description"]);
	}

	factory->initialize(description);

	LoadedScript loadedScript;
	// a stateless script is shared, so it is created once here rather than on every event
	if(description.kind == ScriptKind::COMBAT_EVENT)
	{
		loadedScript.combatEventScript = factory->createCombatEventScript(description.scriptId);

		if(!loadedScript.combatEventScript)
			logMod->error("Scripting host can not provide combat event scripts, required by '%s'!", description.scriptId);
	}
	loadedScript.description = std::move(description);

	registerObject(scope, "script", name, data, scripts.size());
	scripts.push_back(std::move(loadedScript));
}

void ScriptHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	throw std::runtime_error("Not supported");
}

void ScriptHandler::prepareParameters(ScriptID scriptID, JsonNode & parameters, const TextIdentifier & owner) const
{
	const auto * script = find(scriptID);

	if(!script)
		return;

	const auto & description = script->description;

	if(!description.parametersSchema.isNull())
		JsonUtils::validate(parameters, description.parametersSchema, owner.get());

	if(description.stringRegistrations.empty())
		return;

	// without an owner there is no key that stays the same across loads, so a registered string
	// could never be translated. Such callers only get validation.
	if(owner.get().empty())
	{
		logMod->warn("Script '%s' registers translatable strings, but is used by a source that provides no identifier for them!", description.scriptId);
		return;
	}

	for(const auto & field : description.stringRegistrations)
	{
		const JsonNode & fieldNode = static_cast<const JsonNode &>(parameters)[field];
		if(fieldNode.isNull())
			continue;

		const std::string & value = fieldNode.String();
		if(value.empty())
			continue;

		if(value.at(0) == '@')
		{
			// an explicit reference to a string that some other entity already registered
			parameters[field].String() = value.substr(1);
		}
		else
		{
			TextIdentifier textID(owner.get(), field);
			LIBRARY->generaltexth->registerString(fieldNode.getModScope(), textID, fieldNode);
			parameters[field].String() = textID.get();
		}
	}
}
