/*
 * LuaScriptModule.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace spells::effects
{
class LuaSpellEffectFactory;
class LuaUnitEffectFactory;
}

namespace scripting
{

class LuaScriptInstance;

class LuaModule final : public Service
{
public:
	LuaModule();
	~LuaModule();

	void installScripting(spells::effects::SpellEffectService * spellEffects) override;

	std::unique_ptr<Pool> createPoolInstance(const Environment * ENV) const override;

private:
	using ScriptPtr = std::shared_ptr<LuaScriptInstance>;
	using ScriptMap = std::map<std::string, ScriptPtr>;

	std::shared_ptr<spells::effects::LuaSpellEffectFactory> luaSpellEffects;
	std::shared_ptr<spells::effects::LuaUnitEffectFactory> luaUnitEffects;
};
}

VCMI_LIB_NAMESPACE_END
