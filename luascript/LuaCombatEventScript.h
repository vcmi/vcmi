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

#include "../lib/combatScripts/CombatScriptService.h"
#include "../lib/combatScripts/ICombatEventScript.h"

namespace scripting
{
class LuaContext;
class LuaModule;
class LuaScriptInstance;
class LuaScriptPool;

/// Registered under the "lua" combat script type; loads Lua scripts and creates LuaCombatEventScript instances from them.
class LuaCombatScriptFactory final : public ICombatScriptFactory
{
public:
	LuaCombatScriptFactory(LuaModule & host);
	virtual ~LuaCombatScriptFactory();

	void initialize(const std::string & scriptId, const std::string & scope, const std::string & name, const std::vector<PatchEntry> & patches) override;
	std::shared_ptr<ICombatEventScript> get(const std::string & scriptId) const override;

	void registerScripts(LuaScriptPool * pool);

private:
	/// script id -> script mapping
	std::map<std::string, std::unique_ptr<LuaScriptInstance>> loadedScripts;
	/// script id -> shared handler; stateless, so one instance per script is enough
	std::map<std::string, std::shared_ptr<ICombatEventScript>> instances;
	LuaModule & host;
};

/// Dispatches a combat event to the Lua method matching that event, e.g. WAIT to `onWait`.
class LuaCombatEventScript final : public ICombatEventScript
{
public:
	LuaCombatEventScript(const LuaScriptInstance * script);
	virtual ~LuaCombatEventScript();

	void run(ServerCallback * server, const CBattleInfoCallback & battle, CombatEventType event, const battle::Unit * self, const battle::Unit * other, const JsonNode & parameters, const CombatEventPayload & payload) const override;

private:
	const LuaScriptInstance * script;
};

}
