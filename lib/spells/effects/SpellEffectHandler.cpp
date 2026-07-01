/*
 * SpellEffectHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "SpellEffectHandler.h"

#include "../../GameLibrary.h"
#include "../../json/JsonUtils.h"
#include "../../texts/CGeneralTextHandler.h"
#include "../../texts/TextIdentifier.h"

#include "Effect.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells::effects
{

SpellEffectHandler::SpellEffectHandler()
{

}

std::shared_ptr<Effect> SpellEffectHandler::create(SpellEffectID effectID) const
{
	const auto & effect = effectTypes.at(effectID.getNum());
	const auto & factory = effectTypeFactories.at(effect.type);

	return factory->create(effect.effectId);
}

void SpellEffectHandler::registerFactory(const std::string & typeName, std::shared_ptr<ISpellEffectFactory> factory)
{
	effectTypeFactories[typeName] = factory;
}

std::vector<JsonNode> SpellEffectHandler::loadLegacyData()
{
	return {};
}

/// loads single object into game. Scope is namespace of this object, same as name of source mod
void SpellEffectHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	SpellEffectType newEffect;
	newEffect.identifier = name;
	newEffect.modScope = scope;
	newEffect.type = data["type"].String();
	newEffect.scriptName = data["script"].String();
	newEffect.validationSchema = data["schema"];

	for(const auto & item : data["stringRegistrations"].Vector())
		newEffect.stringRegistrations.push_back(item.String());

	const JsonNode & patchesNode = data["patches"];
	if (patchesNode.isVector())
	{
		for (const auto & patchEntry : patchesNode.Vector())
			newEffect.patches.emplace_back(patchEntry.getModScope(), patchEntry.String());
	}

	newEffect.effectId = scope + ':' + name;
	registerObject(scope, "spellEffect", name, data, effectTypes.size());
	effectTypes.push_back(std::move(newEffect));

	const auto & back = effectTypes.back();
	const auto & factory = effectTypeFactories.at(back.type);
	factory->initialize(back.effectId, back.modScope, back.scriptName, back.patches);
}

void SpellEffectHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	throw std::runtime_error("Not supported");
}

void SpellEffectHandler::afterLoadFinalization()
{

}

void SpellEffectHandler::prepareEffect(SpellEffectID effectID, JsonNode & data, const std::string & spellScope, const std::string & spellIdentifier, const std::string & effectName) const
{
	const auto & effectType = effectTypes.at(effectID.getNum());
	const std::string validationName = spellScope + ":" + spellIdentifier + " effect " + effectName;

	if(!effectType.validationSchema.isNull())
		JsonUtils::validate(data, effectType.validationSchema, validationName);

	for(const auto & field : effectType.stringRegistrations)
	{
		const JsonNode & fieldNode = static_cast<const JsonNode &>(data)[field];
		if(fieldNode.isNull())
			continue;
		const std::string & value = fieldNode.String();
		if(value.empty())
			continue;

		if(value.at(0) == '@')
		{
			data[field].String() = value.substr(1);
		}
		else
		{
			TextIdentifier textID("spell", spellScope, spellIdentifier, effectName, field);
			LIBRARY->generaltexth->registerString(spellScope, textID, fieldNode);
			data[field].String() = textID.get();
		}
	}
}

}

VCMI_LIB_NAMESPACE_END
