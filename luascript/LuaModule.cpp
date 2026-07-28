/*
 * LuaScriptModule.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LuaModule.h"

#include "LuaMapEventDispatcher.h"
#include "LuaScriptInstance.h"
#include "LuaScriptPool.h"
#include "LuaSpellEffect.h"

#include "api/DocExport.h"

#include "../lib/GameLibrary.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/mapping/CMap.h"
#include "../lib/modding/ModScope.h"
#include "../lib/spells/effects/SpellEffectService.h"

namespace scripting
{

LuaModule::LuaModule() = default;
LuaModule::~LuaModule() = default;

void LuaModule::installScripting(spells::effects::SpellEffectService * spellEffects)
{
	luaSpellEffects = std::make_shared<spells::effects::LuaSpellEffectFactory>(*this);
	spellEffects->registerFactory("lua", luaSpellEffects);
}

std::unique_ptr<Pool> LuaModule::createPoolInstance(const Environment * ENV) const
{
	auto result = std::make_unique<LuaScriptPool>(*this, ENV);
	luaSpellEffects->registerScripts(result.get());
	return result;
}

std::unique_ptr<MapEventDispatcher> LuaModule::createMapScriptDispatcher(CGameState & gs, bool runInit) const
{
	if(gs.getMap().scriptSource.empty())
		return nullptr;

	auto instance = std::make_shared<LuaScriptInstance>(*this, ModScope::scopeMap(), gs.getMap().scriptSource, std::vector<std::string>{"mapEventRuntime"});
	return std::make_unique<LuaMapEventDispatcher>(instance, gs.getScriptingEnvironment(), gs.getMap(), runInit);
}

void LuaModule::exportDocs(const boost::filesystem::path & outDir) const
{
	api::exportLuaApiDocs(outDir);
}

}
