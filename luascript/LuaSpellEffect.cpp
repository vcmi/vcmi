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

#include "LuaSpellEffect.h"

#include "LuaContext.h"
#include "LuaScriptInstance.h"

#include <vcmi/scripting/Service.h>

#include "../lib/spells/ISpellMechanics.h"
#include "../lib/battle/Unit.h"
#include "../lib/battle/CBattleInfoCallback.h"
#include "../lib/serializer/JsonSerializeFormat.h"

static const std::string APPLICABLE_GENERAL = "applicable";
static const std::string APPLICABLE_TARGET = "applicableTarget";
static const std::string TRANSFORM_TARGET = "transformTarget";
static const std::string APPLY = "apply";

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

LuaSpellEffectFactory::LuaSpellEffectFactory(const LuaScriptInstance * script_)
	: script(script_)
{

}

LuaSpellEffectFactory::~LuaSpellEffectFactory() = default;

Effect * LuaSpellEffectFactory::create() const
{
	return new LuaSpellEffect(script);
}

LuaSpellEffect::LuaSpellEffect(const LuaScriptInstance * script_)
	: script(script_)
{

}

LuaSpellEffect::~LuaSpellEffect() = default;

void LuaSpellEffect::adjustTargetTypes(std::vector<TargetType> & types) const
{

}

void LuaSpellEffect::adjustAffectedHexes(BattleHexArray & hexes, const Mechanics * m, const Target & spellTarget) const
{

}

bool LuaSpellEffect::applicable(Problem & problem, const Mechanics * m) const
{
	std::shared_ptr<LuaContext> context = resolveScript(m);

	bool result = context->callGlobalWithParameters<bool>(APPLICABLE_GENERAL, parameters, m, &problem);

	return result;
}

bool LuaSpellEffect::applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const
{
	std::shared_ptr<scripting::LuaContext> context = resolveScript(m);

	if(target.empty())
		return false;

	bool result = context->callGlobalWithParameters<bool>(APPLICABLE_TARGET, parameters, m, target);

	return result;
}

void LuaSpellEffect::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	std::shared_ptr<scripting::LuaContext> context = resolveScript(m);

	context->callGlobalWithParameters<void>(APPLY, parameters, m, server, target);
}

EffectTarget LuaSpellEffect::filterTarget(const Mechanics * m, const EffectTarget & target) const
{
	return EffectTarget(target);
}

EffectTarget LuaSpellEffect::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	std::shared_ptr<scripting::LuaContext> context = resolveScript(m);

	Target response = context->callGlobalWithParameters<Target>(TRANSFORM_TARGET, parameters, m, aimPoint, spellTarget);

	return response;
}

void LuaSpellEffect::serializeJsonEffect(JsonSerializeFormat & handler)
{
	parameters = handler.getCurrent();
	//TODO: load everything and provide to script
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
