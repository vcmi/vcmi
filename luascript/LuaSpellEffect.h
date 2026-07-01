/*
 * LuaSpellEffect.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../lib/json/JsonNode.h"
#include "../lib/spells/effects/Effect.h"
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

/// Registered under the "lua" effect type; loads Lua scripts on demand and creates LuaSpellEffect instances from them.
class LuaSpellEffectFactory final : public ISpellEffectFactory
{
public:
	LuaSpellEffectFactory(scripting::LuaModule & host);
	virtual ~LuaSpellEffectFactory();

	void initialize(const std::string & effectId,
		const std::string & scope, const std::string & name,
		const std::vector<PatchEntry> & patches) override;
	std::shared_ptr<Effect> create(const std::string & effectId) const override;

	void registerScripts(scripting::LuaScriptPool * pool);

private:
	/// effect id -> script mapping
	std::map<std::string, std::unique_ptr<scripting::LuaScriptInstance> > loadedScripts;
	scripting::LuaModule & host;
};

/// Implements a full spell effect (targeting, applicability, apply) by delegating each step to a Lua script function.
class LuaSpellEffect final : public Effect
{
	using LuaScriptInstance = scripting::LuaScriptInstance;
	using LuaContext = scripting::LuaContext;

public:
	LuaSpellEffect(const LuaScriptInstance * script_);
	virtual ~LuaSpellEffect();

	void adjustTargetTypes(std::vector<TargetType> & types, const Mechanics * m) const override;

	void adjustAffectedHexes(BattleHexArray & hexes, const Mechanics * m, const Target & spellTarget) const override;

	bool applicableGeneral(Problem & problem, const Mechanics * m) const override;
	bool applicableTarget(Problem & problem, const Mechanics * m, const Target & target) const override;

	void apply(ServerCallback * server, const Mechanics * m, const Target & target) const override;

	Target filterTarget(const Mechanics * m, const Target & target) const override;

	Target transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;

	SpellEffectValue getHealthChange(const Mechanics * m, const Target & spellTarget) const override;

protected:
	void initImpl(JsonNode data) override;

private:
	const LuaScriptInstance * script;
	JsonNode parameters;

	std::shared_ptr<LuaContext> resolveScript(const Mechanics * m) const;
};

}

VCMI_LIB_NAMESPACE_END
