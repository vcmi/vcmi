/*
 * LuaCombatEventScript.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaCombatEventScript.h"

#include "LuaContext.h"
#include "LuaScriptInstance.h"
#include "LuaScriptPool.h"

#include <vcmi/ServerCallback.h>

#include "../lib/battle/CBattleInfoCallback.h"
#include "../lib/battle/Unit.h"

namespace scripting
{

/// Combat event to name of the script method handling it. Scripts that do not care about
/// an event simply inherit the no-op implementation from their base class.
/// Switch without default, so that adding a combat event without a handler fails to compile.
static std::string methodName(CombatEventType event)
{
	switch(event)
	{
		case CombatEventType::BEFORE_ATTACK:   return "onBeforeAttack";
		case CombatEventType::AFTER_ATTACK:    return "onAfterAttack";
		case CombatEventType::BEFORE_ATTACKED: return "onBeforeAttacked";
		case CombatEventType::AFTER_ATTACKED:  return "onAfterAttacked";
		case CombatEventType::WAIT:            return "onWait";
		case CombatEventType::DEFEND:          return "onDefend";
		case CombatEventType::BEFORE_MOVE:     return "onBeforeMove";
		case CombatEventType::AFTER_MOVE:      return "onAfterMove";
		case CombatEventType::UNIT_SPELLCAST:  return "onUnitSpellcast";
		case CombatEventType::BATTLE_START:    return "onBattleStart";
		case CombatEventType::ROUND_START:     return "onRoundStart";
		case CombatEventType::ATTACK_RESOLVED: return "onAttackResolved";
		case CombatEventType::INVALID:         break;
	}

	throw std::runtime_error("Combat script called for invalid combat event!");
}

LuaCombatScriptFactory::LuaCombatScriptFactory(LuaModule & host)
	:host(host)
{
}

LuaCombatScriptFactory::~LuaCombatScriptFactory() = default;

void LuaCombatScriptFactory::initialize(const std::string & scriptId, const std::string & scope, const std::string & name, const std::vector<PatchEntry> & patches)
{
	auto loadedScript = std::make_unique<LuaScriptInstance>(host, scope, name, patches);
	instances[scriptId] = std::make_shared<LuaCombatEventScript>(loadedScript.get());
	loadedScripts[scriptId] = std::move(loadedScript);
}

std::shared_ptr<ICombatEventScript> LuaCombatScriptFactory::get(const std::string & scriptId) const
{
	return instances.at(scriptId);
}

void LuaCombatScriptFactory::registerScripts(LuaScriptPool * pool)
{
	for(const auto & script : loadedScripts)
		pool->registerScript(script.second.get());
}

LuaCombatEventScript::LuaCombatEventScript(const LuaScriptInstance * script)
	: script(script)
{
}

LuaCombatEventScript::~LuaCombatEventScript() = default;

void LuaCombatEventScript::run(ServerCallback * server, const CBattleInfoCallback & battle, CombatEventType event, const battle::Unit * self, const battle::Unit * other, const JsonNode & parameters, const CombatEventPayload & payload) const
{
	//TODO: find a way to avoid dynamic casting
	auto genericContext = battle.getScriptContextPool().getContext(script);
	auto luaContext = std::dynamic_pointer_cast<LuaContext>(genericContext);
	if(!luaContext)
		throw std::runtime_error("Failed to execute Lua combat script! Context not available!");

	luaContext->callMethod<void>(methodName(event), parameters, server, &battle, self, other, payload);
}

}
