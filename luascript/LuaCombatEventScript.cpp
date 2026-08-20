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

namespace
{

const char * methodName(CombatEventType event)
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
		case CombatEventType::BATTLE_SETUP:    return "onBattleSetup";
		case CombatEventType::BATTLE_START:    return "onBattleStart";
		case CombatEventType::ROUND_START:     return "onRoundStart";
		case CombatEventType::INVALID:         break;
	}

	throw std::runtime_error("Combat script called for invalid combat event!");
}

}

LuaCombatEventScript::LuaCombatEventScript(const LuaScriptInstance * script)
	: script(script)
{
}

LuaCombatEventScript::~LuaCombatEventScript() = default;

std::shared_ptr<LuaContext> LuaCombatEventScript::contextOf(const CBattleInfoCallback & battle) const
{
	return LuaContext::of(battle.getScriptContextPool(), script);
}

bool LuaCombatEventScript::handlesEvent(const CBattleInfoCallback & battle, CombatEventType event) const
{
	return contextOf(battle)->hasFunction(methodName(event));
}

void LuaCombatEventScript::run(ServerCallback * server, const CBattleInfoCallback & battle, CombatEventType event, const battle::Unit * self, const battle::Unit * other, const JsonNode & parameters, const CombatEventPayload & payload) const
{
	contextOf(battle)->callMethod<void>(methodName(event), parameters, server, &battle, self, other, payload);
}

}
