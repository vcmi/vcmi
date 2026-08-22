/*
 * LuaDamageCalculatorScript.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../lib/combatScripts/IDamageCalculatorScript.h"
#include "../lib/bonuses/BonusEnum.h"

namespace scripting
{
class LuaContext;
class LuaScriptInstance;

/// Asks the script what one attack is worth, in a single call per estimate.
class LuaDamageCalculatorScript final : public IDamageCalculatorScript
{
public:
	LuaDamageCalculatorScript(const LuaScriptInstance * script);
	~LuaDamageCalculatorScript() override;

	DamageEstimation calculate(const CBattleInfoCallback & battle, DamageAttackInfo & info) const override;

private:
	const LuaScriptInstance * script;

	/// Bonus types the script declares an interest in, with the key each is reported under. Asked
	/// for once - the answer is a property of the script, and the script does not change.
	mutable std::once_flag declaredOnce;
	mutable std::unordered_map<BonusType, std::string> declared;

	void ensureDeclared(const CBattleInfoCallback & battle) const;
	/// Which of the declared types this unit carries, as the lookup table the script reads.
	std::unordered_map<std::string, bool> carriedBonuses(const battle::Unit * unit) const;

	std::shared_ptr<LuaContext> contextOf(const CBattleInfoCallback & battle) const;
};

}
