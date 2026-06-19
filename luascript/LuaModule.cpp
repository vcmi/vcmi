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

#include "LuaScriptInstance.h"
#include "LuaScriptPool.h"
#include "LuaSpellEffect.h"

#include "api/DocExport.h"

#include "../lib/GameLibrary.h"
#include "../lib/spells/effects/SpellEffectService.h"

#ifdef __GNUC__
#	define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

static const char * const g_cszAiName = "Lua interpreter";

VCMI_LIB_NAMESPACE_BEGIN

extern "C" DLL_EXPORT void GetAiName(char * name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_EXPORT void GetNewModule(std::unique_ptr<scripting::Service> & out)
{
	out = std::make_unique<scripting::LuaModule>();
}

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

void LuaModule::exportDocs(const boost::filesystem::path & outDir) const
{
	api::exportLuaApiDocs(outDir);
}

}

VCMI_LIB_NAMESPACE_END
