/*
 * LuaDamageCalculatorScript.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaDamageCalculatorScript.h"

#include "LuaContext.h"
#include "LuaScriptInstance.h"
#include "LuaScriptPool.h"

#include "../lib/battle/CBattleInfoCallback.h"
#include "../lib/battle/Unit.h"
#include "../lib/bonuses/BonusList.h"
#include "../lib/bonuses/BonusSelector.h"
#include "../lib/CBonusTypeHandler.h"
#include "../lib/GameLibrary.h"

namespace scripting
{

LuaDamageCalculatorScript::LuaDamageCalculatorScript(const LuaScriptInstance * script)
	: script(script)
{
}

LuaDamageCalculatorScript::~LuaDamageCalculatorScript() = default;

std::shared_ptr<LuaContext> LuaDamageCalculatorScript::contextOf(const CBattleInfoCallback & battle) const
{
	//TODO: find a way to avoid dynamic casting
	auto genericContext = battle.getScriptContextPool().getContext(script);
	auto luaContext = std::dynamic_pointer_cast<LuaContext>(genericContext);
	if(!luaContext)
		throw std::runtime_error("Failed to execute Lua damage calculator! Context not available!");

	return luaContext;
}

void LuaDamageCalculatorScript::ensureDeclared(const CBattleInfoCallback & battle) const
{
	std::call_once(declaredOnce, [this, &battle]()
	{
		static const JsonNode noParameters;

		auto names = contextOf(battle)->callMethod<std::vector<std::string>>("bonusTypes", noParameters);

		for(const auto & name : names)
			declared.emplace_back(static_cast<BonusType>(BonusTypeID::decode(name)), name);

		if(declared.empty())
			logMod->warn("Damage calculator declares no bonus types! Every factor that gates on one will find nothing.");
	});
}

std::map<std::string, bool> LuaDamageCalculatorScript::carriedBonuses(const battle::Unit * unit) const
{
	std::set<BonusType> present;

	// one pass over the bonuses of the unit rather than a query per declared type, of which there
	// are two dozen. Held rather than iterated in place: the list is shared and would be freed first
	const auto bonuses = unit->getAllBonuses(Selector::all);

	for(const auto & bonus : *bonuses)
		present.insert(bonus->type);

	std::map<std::string, bool> result;

	// only what the unit carries is worth sending: of the two dozen types the script declares an
	// interest in, a unit has about three. Whether a type was declared at all is something the
	// script knows on its own, so nothing is gained by reporting the absent ones
	for(const auto & [type, name] : declared)
		if(present.count(type) != 0)
			result.emplace(name, true);

	return result;
}

DamageEstimation LuaDamageCalculatorScript::calculate(const CBattleInfoCallback & battle, DamageAttackInfo & info) const
{
	static const JsonNode noParameters;

	ensureDeclared(battle);

	info.attackerBonuses = carriedBonuses(info.attacker);
	info.defenderBonuses = carriedBonuses(info.defender);

	auto answer = contextOf(battle)->callMethod<DamageEstimationPayload>("calculate", noParameters, &battle, info);

	return DamageEstimation{
		{answer.damage.min, answer.damage.max},
		{answer.kills.min, answer.kills.max},
		{answer.damageBeforeDefense.min, answer.damageBeforeDefense.max}
	};
}

}
