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
#include "../lib/spells/effects/Registry.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

namespace scripting
{
class LuaScriptInstance;
class LuaContext;
}

namespace spells
{
namespace effects
{



class LuaSpellEffectFactory : public IEffectFactory
{
	using LuaScriptInstance = scripting::LuaScriptInstance;
	using LuaContext = scripting::LuaContext;

public:
	LuaSpellEffectFactory(const LuaScriptInstance * script_);
	virtual ~LuaSpellEffectFactory();

	Effect * create() const override;

private:
	const LuaScriptInstance * script;
};

class LuaSpellEffect : public Effect
{
	using LuaScriptInstance = scripting::LuaScriptInstance;
	using LuaContext = scripting::LuaContext;

public:
	LuaSpellEffect(const LuaScriptInstance * script_);
	virtual ~LuaSpellEffect();

	void adjustTargetTypes(std::vector<TargetType> & types) const override;

	void adjustAffectedHexes(BattleHexArray & hexes, const Mechanics * m, const Target & spellTarget) const override;

	bool applicable(Problem & problem, const Mechanics * m) const override;
	bool applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const override;

	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget filterTarget(const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;

protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override;

private:
	const LuaScriptInstance * script;
	JsonNode parameters;

	std::shared_ptr<LuaContext> resolveScript(const Mechanics * m) const;

	void setContextVariables(const Mechanics * m, const std::shared_ptr<LuaContext>& context) const;
	JsonNode spellTargetToJson(const Target & spellTarget) const;
	Target spellTargetFromJson(const Mechanics * m, const JsonNode & config) const;
};

}
}

VCMI_LIB_NAMESPACE_END
