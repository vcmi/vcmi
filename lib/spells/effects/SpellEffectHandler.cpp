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

#include "../../json/JsonUtils.h"

#include "Catapult.h"
#include "Clone.h"
#include "Damage.h"
#include "Dispel.h"
#include "Effect.h"
#include "Heal.h"
#include "Moat.h"
#include "Obstacle.h"
#include "RemoveObstacle.h"
#include "Sacrifice.h"
#include "Teleport.h"
#include "Timed.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells::effects
{

void BuiltinEffectFactory::initialize(const std::string & scope, const std::string & name)
{}

template <typename T>
std::shared_ptr<Effect> makeEffect()
{
	return std::make_shared<T>();
}

std::shared_ptr<Effect> BuiltinEffectFactory::create(const std::string & scope, const std::string & name) const
{
	using EffectFactoryFunctor = std::shared_ptr<Effect>(*)();

	static const std::unordered_map<std::string, EffectFactoryFunctor> effectFactory = {
		{ "Catapult",       &makeEffect<Catapult> },
		{ "Clone",          &makeEffect<Clone> },
		{ "Damage",         &makeEffect<Damage> },
		{ "Dispel",         &makeEffect<Dispel> },
		{ "Heal",           &makeEffect<Heal> },
		{ "Moat",           &makeEffect<Moat> },
		{ "Obstacle",       &makeEffect<Obstacle> },
		{ "RemoveObstacle", &makeEffect<RemoveObstacle> },
		{ "Sacrifice",      &makeEffect<Sacrifice> },
		{ "Teleport",       &makeEffect<Teleport> },
		{ "Timed",          &makeEffect<Timed> },
	};

	return effectFactory.at(name)();
}

SpellEffectHandler::SpellEffectHandler()
{
	registerFactory("builtin", std::make_unique<BuiltinEffectFactory>());
}

std::shared_ptr<Effect> SpellEffectHandler::create(SpellEffectID effectID) const
{
	const auto & effect = effectTypes.at(effectID.getNum());
	const auto & factory = effectTypeFactories.at(effect.type);

	return factory->create(effect.modScope, effect.scriptName);
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

	registerObject(scope, "spellEffect", name, data, effectTypes.size());
	effectTypes.push_back(newEffect);

	const auto & factory = effectTypeFactories.at(newEffect.type);
	factory->initialize(newEffect.modScope, newEffect.scriptName);
}

void SpellEffectHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	throw std::runtime_error("Not supported");
}

void SpellEffectHandler::afterLoadFinalization()
{

}

void SpellEffectHandler::validateEffect(SpellEffectID effectID, const JsonNode & data, const std::string & name) const
{
	const auto & schema = effectTypes.at(effectID.getNum()).validationSchema;
	if (!schema.isNull())
		JsonUtils::validate(data, schema, name);
}

}

VCMI_LIB_NAMESPACE_END
