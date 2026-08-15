/*
 * LuaCombatEventScript.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../lib/combatScripts/ICombatEventScript.h"

namespace scripting
{
class LuaContext;
class LuaScriptInstance;

/// Dispatches a combat event to the Lua method matching that event, e.g. WAIT to `onWait`.
class LuaCombatEventScript final : public ICombatEventScript
{
public:
	LuaCombatEventScript(const LuaScriptInstance * script);
	~LuaCombatEventScript() override;

	bool handlesEvent(const CBattleInfoCallback & battle, CombatEventType event) const override;
	void run(ServerCallback * server, const CBattleInfoCallback & battle, CombatEventType event, const battle::Unit * self, const battle::Unit * other, const JsonNode & parameters, const CombatEventPayload & payload) const override;

private:
	const LuaScriptInstance * script;

	std::shared_ptr<LuaContext> contextOf(const CBattleInfoCallback & battle) const;
};

}
