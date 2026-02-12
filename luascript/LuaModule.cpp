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

#include "../lib/GameLibrary.h"
#include "../lib/spells/effects/Registry.h"

#ifdef __GNUC__
#	define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

static const char * const g_cszAiName = "Lua interpreter";

VCMI_LIB_NAMESPACE_BEGIN

extern "C" DLL_EXPORT void GetAiName(char * name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_EXPORT void GetNewModule(std::shared_ptr<scripting::Module> & out)
{
	out = std::make_shared<scripting::LuaModule>();
}

namespace scripting
{

LuaModule::LuaModule() = default;
LuaModule::~LuaModule() = default;

LuaModule::ScriptPtr LuaModule::loadFromJson(vstd::CLoggerBase * logger, const std::string & scope, const JsonNode & json, const std::string & identifier)
{
	auto ret = std::make_shared<LuaScriptInstance>(*this);

	ret->identifier = identifier;
	ret->modScope = scope;
	ret->loadFromJson(json);
	return ret;
}

void LuaModule::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(logMod, scope, data, name);
	objects[object->getJsonKey()] = object;
}

void LuaModule::afterLoadFinalization()
{
	for(const auto & [name, script] : objects)
	{
		switch(script->implements)
		{
			case LuaScriptInstance::Implements::ANYTHING:
				break;
			case LuaScriptInstance::Implements::BATTLE_EFFECT:
				LIBRARY->spellEffects()->add(script->getJsonKey(), std::make_shared<spells::effects::LuaSpellEffectFactory>(script.get()));
				break;
		}
	}
}

std::unique_ptr<Pool> LuaModule::createPoolInstance(const Environment * ENV) const
{
	auto result = std::make_unique<LuaScriptPool>(*this, ENV);

	for(const auto & [name, script] : objects)
		result->registerScript(script.get());

	return result;
}
}

VCMI_LIB_NAMESPACE_END
