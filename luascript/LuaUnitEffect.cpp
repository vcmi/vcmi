/*
 * LuaUnitEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaScriptPool.h"
#include "LuaUnitEffect.h"

#include "LuaContext.h"
#include "LuaScriptInstance.h"

#include <vcmi/scripting/Service.h>

#include "../lib/spells/ISpellMechanics.h"
#include "../lib/battle/Unit.h"
#include "../lib/battle/CBattleInfoCallback.h"
#include "../lib/serializer/JsonSerializeFormat.h"

static const std::string APPLY = "apply";
static const std::string IS_RECEPTIVE = "isReceptive";
static const std::string IS_VALID_TARGET = "isValidTarget";
static const std::string GET_HEALTH_CHANGE = "getHealthChange";

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

LuaUnitEffectFactory::LuaUnitEffectFactory(scripting::LuaModule & host)
	: host(host)
{
}

LuaUnitEffectFactory::~LuaUnitEffectFactory() = default;

void LuaUnitEffectFactory::initialize(const std::string & scope, const std::string & name)
{
	auto loadedScript = std::make_unique<scripting::LuaScriptInstance>(host, scope, name);
	loadedScripts[loadedScript->getIdentifier()] = std::move(loadedScript);
}

std::shared_ptr<Effect> LuaUnitEffectFactory::create(const std::string & scope, const std::string & name) const
{
	return std::make_shared<LuaUnitEffect>(loadedScripts.at(scope + ':' + name).get());
}

void LuaUnitEffectFactory::registerScripts(scripting::LuaScriptPool * pool)
{
	for(const auto & script : loadedScripts)
		pool->registerScript(script.second.get());
}

LuaUnitEffect::LuaUnitEffect(const LuaScriptInstance * script_)
	: script(script_)
{
}

LuaUnitEffect::~LuaUnitEffect() = default;

void LuaUnitEffect::apply(ServerCallback * server, const Mechanics * m, const Target & target) const
{
	std::shared_ptr<LuaContext> context = resolveScript(m);
	context->callMethod<void>(APPLY, parameters, m, server, target);
}

bool LuaUnitEffect::isReceptive(const Mechanics * m, const battle::Unit * unit) const
{
	std::shared_ptr<LuaContext> context = resolveScript(m);
	return context->callMethod<bool>(IS_RECEPTIVE, parameters, m, unit);
}

bool LuaUnitEffect::isValidTarget(const Mechanics * m, const battle::Unit * unit) const
{
	std::shared_ptr<LuaContext> context = resolveScript(m);
	return context->callMethod<bool>(IS_VALID_TARGET, parameters, m, unit);
}

SpellEffectValue LuaUnitEffect::getHealthChange(const Mechanics * m, const Target & spellTarget) const
{
	std::shared_ptr<LuaContext> context = resolveScript(m);
	return context->callMethod<SpellEffectValue>(GET_HEALTH_CHANGE, parameters, m, spellTarget);
}

void LuaUnitEffect::serializeJsonUnitEffect(JsonSerializeFormat & handler)
{
	parameters = handler.getCurrent();
}

std::shared_ptr<scripting::LuaContext> LuaUnitEffect::resolveScript(const Mechanics * m) const
{
	auto genericContext = m->battle()->getScriptContextPool().getContext(script);
	auto luaContext = std::dynamic_pointer_cast<scripting::LuaContext>(genericContext);
	if(!luaContext)
		throw std::runtime_error("Failed to execute Lua script unit effect! Context not available!");

	return luaContext;
}

}
}

VCMI_LIB_NAMESPACE_END
