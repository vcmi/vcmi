/*
 * LuaSpellEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaScriptPool.h"
#include "LuaSpellEffect.h"

#include "LuaContext.h"
#include "LuaScriptInstance.h"

#include <vcmi/scripting/Service.h>

#include "../lib/spells/ISpellMechanics.h"
#include "../lib/battle/Unit.h"
#include "../lib/battle/CBattleInfoCallback.h"

static const std::string APPLICABLE_GENERAL = "applicableGeneral";
static const std::string APPLICABLE_TARGET = "applicableTarget";
static const std::string FILTER_TARGET = "filterTarget";
static const std::string TRANSFORM_TARGET = "transformTarget";
static const std::string APPLY = "apply";
static const std::string GET_HEALTH_CHANGE = "getHealthChange";
static const std::string ADJUST_AFFECTED_HEXES = "adjustAffectedHexes";
static const std::string ADJUST_TARGET_TYPES = "adjustTargetTypes";

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

LuaSpellEffectFactory::LuaSpellEffectFactory(scripting::LuaModule & host)
	:host(host)
{

}

LuaSpellEffectFactory::~LuaSpellEffectFactory() = default;

void LuaSpellEffectFactory::initialize(const std::string & effectId,
	const std::string & scope, const std::string & name,
	const std::vector<PatchEntry> & patches)
{
	auto loadedScript = std::make_unique<scripting::LuaScriptInstance>(host, scope, name, patches);
	loadedScripts[effectId] = std::move(loadedScript);
}

std::shared_ptr<Effect> LuaSpellEffectFactory::create(const std::string & effectId) const
{
	return std::make_shared<LuaSpellEffect>(loadedScripts.at(effectId).get());
}

void LuaSpellEffectFactory::registerScripts(scripting::LuaScriptPool * pool)
{
	for (const auto & script : loadedScripts)
		pool->registerScript(script.second.get());
}

LuaSpellEffect::LuaSpellEffect(const LuaScriptInstance * script_)
	: script(script_)
{

}

LuaSpellEffect::~LuaSpellEffect() = default;

void LuaSpellEffect::adjustTargetTypes(std::vector<TargetType> & types, const Mechanics * m) const
{
	std::shared_ptr<LuaContext> context = resolveScript(m);
	types = context->callMethod<std::vector<TargetType>>(ADJUST_TARGET_TYPES, parameters, m, types);
}

void LuaSpellEffect::adjustAffectedHexes(BattleHexArray & hexes, const Mechanics * m, const Target & spellTarget) const
{
	std::shared_ptr<LuaContext> context = resolveScript(m);
	context->callMethod<void>(ADJUST_AFFECTED_HEXES, parameters, m, hexes, spellTarget);
}

SpellEffectValue LuaSpellEffect::getHealthChange(const Mechanics * m, const Target & spellTarget) const
{
	std::shared_ptr<LuaContext> context = resolveScript(m);
	auto result = context->callMethod<SpellEffectValue>(GET_HEALTH_CHANGE, parameters, m, spellTarget);
	return result;
}

bool LuaSpellEffect::applicableGeneral(Problem & problem, const Mechanics * m) const
{
	std::shared_ptr<LuaContext> context = resolveScript(m);
	bool result = context->callMethod<bool>(APPLICABLE_GENERAL, parameters, m, &problem);
	return result;
}

bool LuaSpellEffect::applicableTarget(Problem & problem, const Mechanics * m, const Target & target) const
{
	std::shared_ptr<scripting::LuaContext> context = resolveScript(m);
	bool result = context->callMethod<bool>(APPLICABLE_TARGET, parameters, m, &problem, target);
	return result;
}

void LuaSpellEffect::apply(ServerCallback * server, const Mechanics * m, const Target & target) const
{
	std::shared_ptr<scripting::LuaContext> context = resolveScript(m);
	context->callMethod<void>(APPLY, parameters, m, server, target);
}

Target LuaSpellEffect::filterTarget(const Mechanics * m, const Target & target) const
{
	std::shared_ptr<scripting::LuaContext> context = resolveScript(m);
	return context->callMethod<Target>(FILTER_TARGET, parameters, m, target);
}

Target LuaSpellEffect::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	std::shared_ptr<scripting::LuaContext> context = resolveScript(m);
	Target response = context->callMethod<Target>(TRANSFORM_TARGET, parameters, m, aimPoint, spellTarget);
	return response;
}

void LuaSpellEffect::initImpl(JsonNode data)
{
	parameters = std::move(data);
}

std::shared_ptr<scripting::LuaContext> LuaSpellEffect::resolveScript(const Mechanics * m) const
{
	//TODO: find a way to avoid dynamic casting
	auto genericContext = m->battle()->getScriptContextPool().getContext(script);
	auto luaContext = std::dynamic_pointer_cast<scripting::LuaContext>(genericContext);
	if(!luaContext)
		throw std::runtime_error("Failed to execute Lua script effect! Context not available!");

	return luaContext;
}

}
}

VCMI_LIB_NAMESPACE_END
