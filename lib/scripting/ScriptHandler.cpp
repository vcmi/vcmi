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
#include "../modding/IdentifierStorage.h"
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
	if(name == "damageCalculator")
		return ScriptKind::DAMAGE_CALCULATOR;

	return ScriptKind::INVALID;
}

}

const ScriptTypeDescription & ScriptHandler::getById(ScriptID scriptID) const
{
	return scripts.at(scriptID.getNum());
}

std::shared_ptr<spells::effects::Effect> ScriptHandler::createSpellEffect(ScriptID scriptID) const
{
	const ScriptTypeDescription & description = getById(scriptID);
	auto effect = factory->createSpellEffect(description.scriptId);

	if(!effect)
		logMod->error("Scripting host can not provide spell effects, required by '%s'!", description.scriptId);

	return effect;
}

const IDamageCalculatorScript * ScriptHandler::getDamageCalculator() const
{
	return damageCalculator.get();
}

void ScriptHandler::registerFactory(std::shared_ptr<IScriptFactory> newFactory)
{
	factory = std::move(newFactory);
}

std::vector<JsonNode> ScriptHandler::loadLegacyData()
{
	return {};
}

void ScriptHandler::loadObject(const std::string & scope, const std::string & name, const JsonNode & data)
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

	// a stateless script is shared, so it is created once here rather than on every event
	if(description.kind == ScriptKind::COMBAT_EVENT)
	{
		description.combatEventScript = factory->createCombatEventScript(description.scriptId);

		// every unit carrying this ability would deref that null the moment it acts, so there is
		// nothing to gain by carrying on with the load
		if(!description.combatEventScript)
			throw std::runtime_error("Scripting host can not provide combat event scripts, required by '" + description.scriptId + "'!");
	}

	if(description.kind == ScriptKind::DAMAGE_CALCULATOR)
	{
		description.damageCalculatorScript = factory->createDamageCalculatorScript(description.scriptId);

		if(!description.damageCalculatorScript)
			throw std::runtime_error("Scripting host can not provide damage calculator scripts, required by '" + description.scriptId + "'!");

		// the game has one damage calculator, so a second declaration is a mod fighting the one
		// already in place. Extending it is what `patches` is for
		if(damageCalculator)
			logMod->error("Script '%s' declares a second damage calculator, which will be ignored!", description.scriptId);
		else
			damageCalculator = description.damageCalculatorScript;
	}

	registerObject(scope, "script", name, data, scripts.size());
	scripts.push_back(std::move(description));
}

void ScriptHandler::loadObject(const std::string & scope, const std::string & name, const JsonNode & data, size_t index)
{
	throw std::runtime_error("Not supported");
}

void ScriptHandler::prepareParameters(ScriptID scriptID, JsonNode & parameters, const TextIdentifier & owner) const
{
	const ScriptTypeDescription & description = getById(scriptID);

	if(!description.parametersSchema.isNull())
		JsonUtils::validate(parameters, description.parametersSchema, owner.get());

	// a parameter naming an entity is resolved here purely to have it reported when it names
	// nothing - otherwise a typo only shows up as a raw json key in the creature window
	for(const auto & [field, property] : description.parametersSchema["properties"].Struct())
	{
		const std::string & entityType = property["entity"].String();

		if(!entityType.empty())
			LIBRARY->identifiers()->requestIdentifierIfNotNull(entityType, static_cast<const JsonNode &>(parameters)[field], [](si32){});
	}

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
