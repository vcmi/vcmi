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

class JsonNode;

namespace scripting
{
class LuaScriptInstance;
class LuaContext;
}

namespace spells::effects
{

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
