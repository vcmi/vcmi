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

VCMI_LIB_NAMESPACE_BEGIN

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
