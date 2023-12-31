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

#include "../../lib/spells/effects/Effect.h"
#include "../../lib/spells/effects/Registry.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
	class Script;
	class Context;
}

namespace spells
{
namespace effects
{

using scripting::Script;
using scripting::Context;

class LuaSpellEffectFactory : public IEffectFactory
{
public:
	LuaSpellEffectFactory(const Script * script_);
	virtual ~LuaSpellEffectFactory();

	virtual Effect * create() const override;

private:
	const Script * script;
};

class LuaSpellEffect : public Effect
{
public:
	LuaSpellEffect(const Script * script_);
	virtual ~LuaSpellEffect();

	void adjustTargetTypes(std::vector<TargetType> & types) const override;

	void adjustAffectedHexes(std::set<BattleHex> & hexes, const Mechanics * m, const Target & spellTarget) const override;

	bool applicable(Problem & problem, const Mechanics * m) const override;
	bool applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const override;

	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget filterTarget(const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;

protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override;

private:
	const Script * script;

	std::shared_ptr<Context> resolveScript(const Mechanics * m) const;

	static void setContextVariables(const Mechanics * m, const std::shared_ptr<Context>& context) ;
};

}
}

VCMI_LIB_NAMESPACE_END
