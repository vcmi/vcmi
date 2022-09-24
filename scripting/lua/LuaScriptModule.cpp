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

#include "LuaScriptModule.h"
#include "LuaScriptingContext.h"
#include "LuaSpellEffect.h"

#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

const char *g_cszAiName = "Lua interpreter";

VCMI_LIB_NAMESPACE_BEGIN

extern "C" DLL_EXPORT void GetAiName(char * name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_EXPORT void GetNewModule(std::shared_ptr<scripting::Module> & out)
{
	out = std::make_shared<scripting::LuaScriptModule>();
}

namespace scripting
{

LuaScriptModule::LuaScriptModule() = default;
LuaScriptModule::~LuaScriptModule() = default;

std::string LuaScriptModule::compile(const std::string & name, const std::string & source, vstd::CLoggerBase * logger) const
{
	//TODO: pre-compile to byte code
	//LuaJit bytecode in architecture agnostic, but is not backward compatible and completely incompatible with Lua
	return source;
}

std::shared_ptr<ContextBase> LuaScriptModule::createContextFor(const Script * source, const Environment * env) const
{
	return std::make_shared<LuaContext>(source, env);
}

void LuaScriptModule::registerSpellEffect(spells::effects::Registry * registry, const Script * source) const
{
	registry->add(source->getName(), std::make_shared<spells::effects::LuaSpellEffectFactory>(source));
}


}

VCMI_LIB_NAMESPACE_END
