/*
 * LuaScriptFactory.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../lib/scripting/ScriptService.h"

namespace scripting
{

class LuaModule;
class LuaScriptInstance;
class LuaScriptPool;

/// Every Lua script the game has loaded, of every kind. Kinds differ only in which object is
/// built on top of a script, so they all share one store and one registration into the pool.
class LuaScriptStore
{
public:
	LuaScriptStore(LuaModule & host);
	~LuaScriptStore();

	void load(const ScriptTypeDescription & description);

	const LuaScriptInstance * get(const std::string & scriptId) const;

	void registerScripts(LuaScriptPool * pool) const;

private:
	/// script id -> loaded source layers
	std::map<std::string, std::unique_ptr<LuaScriptInstance>> loadedScripts;
	LuaModule & host;
};

/// Wraps a loaded Lua script into whichever object the kind of that script calls for.
class LuaScriptFactory final : public IScriptFactory
{
public:
	LuaScriptFactory(LuaScriptStore & store);

	void initialize(const ScriptTypeDescription & description) override;

	std::shared_ptr<ICombatEventScript> createCombatEventScript(const std::string & scriptId) const override;
	std::shared_ptr<IDamageCalculatorScript> createDamageCalculatorScript(const std::string & scriptId) const override;
	std::shared_ptr<spells::effects::Effect> createSpellEffect(const std::string & scriptId) const override;

private:
	LuaScriptStore & store;
};

}
