/*
 * LuaUnitEffect.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../lib/json/JsonNode.h"
#include "../lib/spells/effects/UnitEffect.h"
#include "../lib/spells/effects/SpellEffectService.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

namespace scripting
{
class LuaScriptInstance;
class LuaContext;
class LuaModule;
class LuaScriptPool;
}

namespace spells::effects
{

class LuaUnitEffectFactory final : public ISpellEffectFactory
{
public:
	LuaUnitEffectFactory(scripting::LuaModule & host);
	virtual ~LuaUnitEffectFactory();

	void initialize(const std::string & scope, const std::string & name) override;
	std::shared_ptr<Effect> create(const std::string & scope, const std::string & name) const override;

	void registerScripts(scripting::LuaScriptPool * pool);

private:
	std::map<std::string, std::unique_ptr<scripting::LuaScriptInstance>> loadedScripts;
	scripting::LuaModule & host;
};

class LuaUnitEffect final : public UnitEffect
{
	using LuaScriptInstance = scripting::LuaScriptInstance;
	using LuaContext = scripting::LuaContext;

public:
	LuaUnitEffect(const LuaScriptInstance * script_);
	virtual ~LuaUnitEffect();

	void apply(ServerCallback * server, const Mechanics * m, const Target & target) const override;

	SpellEffectValue getHealthChange(const Mechanics * m, const Target & spellTarget) const override;

protected:
	bool isReceptive(const Mechanics * m, const battle::Unit * unit) const override;
	bool isValidTarget(const Mechanics * m, const battle::Unit * unit) const override;

	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override;

private:
	const LuaScriptInstance * script;
	JsonNode parameters;

	std::shared_ptr<LuaContext> resolveScript(const Mechanics * m) const;
};

}

VCMI_LIB_NAMESPACE_END
