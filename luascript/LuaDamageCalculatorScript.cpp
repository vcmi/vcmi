/*
 * LuaDamageCalculatorScript.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaDamageCalculatorScript.h"

#include "LuaContext.h"
#include "LuaScriptInstance.h"
#include "LuaScriptPool.h"

#include "../lib/battle/CBattleInfoCallback.h"
#include "../lib/battle/Unit.h"

namespace scripting
{

LuaDamageCalculatorScript::LuaDamageCalculatorScript(const LuaScriptInstance * script)
	: script(script)
{
}

LuaDamageCalculatorScript::~LuaDamageCalculatorScript() = default;

std::shared_ptr<LuaContext> LuaDamageCalculatorScript::contextOf(const CBattleInfoCallback & battle) const
{
	//TODO: find a way to avoid dynamic casting
	auto genericContext = battle.getScriptContextPool().getContext(script);
	auto luaContext = std::dynamic_pointer_cast<LuaContext>(genericContext);
	if(!luaContext)
		throw std::runtime_error("Failed to execute Lua damage calculator! Context not available!");

	return luaContext;
}

DamageEstimation LuaDamageCalculatorScript::calculate(const CBattleInfoCallback & battle, const DamageAttackInfo & info) const
{
	static const JsonNode noParameters;

	auto answer = contextOf(battle)->callMethod<DamageEstimationPayload>("calculate", noParameters, &battle, info);

	return DamageEstimation{
		{answer.damage.min, answer.damage.max},
		{answer.kills.min, answer.kills.max},
		{answer.damageBeforeDefense.min, answer.damageBeforeDefense.max}
	};
}

}
