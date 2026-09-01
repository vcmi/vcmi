/*
 * LuaScriptFactory.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaScriptFactory.h"

#include "LuaCombatEventScript.h"
#include "LuaDamageCalculatorScript.h"
#include "LuaScriptInstance.h"
#include "LuaScriptPool.h"
#include "LuaSpellEffect.h"

namespace scripting
{

LuaScriptStore::LuaScriptStore(LuaModule & host)
	: host(host)
{
}

LuaScriptStore::~LuaScriptStore() = default;

void LuaScriptStore::load(const ScriptTypeDescription & description)
{
	ScriptPath basePath = ScriptPath::builtinTODO(description.sourcePath).addPrefix("SCRIPTS/");
	loadedScripts[description.scriptId] = std::make_unique<LuaScriptInstance>(host, description.modScope, basePath, description.patches);
}

const LuaScriptInstance * LuaScriptStore::get(const std::string & scriptId) const
{
	return loadedScripts.at(scriptId).get();
}

void LuaScriptStore::registerScripts(LuaScriptPool * pool) const
{
	for(const auto & script : loadedScripts)
		pool->registerScript(script.second.get());
}

LuaScriptFactory::LuaScriptFactory(LuaScriptStore & store)
	: store(store)
{
}

void LuaScriptFactory::initialize(const ScriptTypeDescription & description)
{
	store.load(description);
}

std::shared_ptr<ICombatEventScript> LuaScriptFactory::createCombatEventScript(const std::string & scriptId) const
{
	return std::make_shared<LuaCombatEventScript>(store.get(scriptId));
}

std::shared_ptr<IDamageCalculatorScript> LuaScriptFactory::createDamageCalculatorScript(const std::string & scriptId) const
{
	return std::make_shared<LuaDamageCalculatorScript>(store.get(scriptId));
}

std::shared_ptr<spells::effects::Effect> LuaScriptFactory::createSpellEffect(const std::string & scriptId) const
{
	return std::make_shared<spells::effects::LuaSpellEffect>(store.get(scriptId));
}

}
