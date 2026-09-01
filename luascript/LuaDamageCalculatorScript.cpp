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
#include "../lib/bonuses/BonusFilter.h"
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
	return LuaContext::of(battle.getScriptContextPool(), script);
}

void LuaDamageCalculatorScript::ensureDeclared(const CBattleInfoCallback & battle) const
{
	std::call_once(declaredOnce, [this, &battle]()
	{
		static const JsonNode noParameters;

		auto names = contextOf(battle)->callMethod<std::vector<std::string>>("bonusTypes", noParameters);

		for(const auto & name : names)
		{
			// a script naming a type that does not exist is a mod's typo, and costs it only the one
			// factor that reads it - the rest of the calculator has no business failing over it
			try
			{
				declared.try_emplace(static_cast<BonusType>(BonusTypeID::decode(name)), name);
			}
			catch(const IdentifierResolutionException &)
			{
				logMod->error("Damage calculator declares an interest in bonus '%s', which is no bonus type! Every factor reading it will find nothing.", name);
			}
		}

		if(declared.empty())
			logMod->warn("Damage calculator declares no bonus types! Every factor that gates on one will find nothing.");
	});
}

std::unordered_map<std::string, bool> LuaDamageCalculatorScript::carriedBonuses(const battle::Unit * unit) const
{
	// asked once per unit per attack, so it is worth having the bonus system keep the answer. The
	// key comes from an empty filter, which is what a script asking for every bonus compiles to -
	// both then share the one cached list
	static const auto allBonuses = BonusFilter{}.compile();

	std::unordered_map<std::string, bool> result;

	const auto bonuses = unit->getAllBonuses(allBonuses.first, allBonuses.second);
	for(const auto & bonus : *bonuses)
	{
		auto entry = declared.find(bonus->type);

		if(entry != declared.end())
			result.try_emplace(entry->second, true);
	}

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
