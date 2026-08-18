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

	DamageEstimation calculate(const CBattleInfoCallback & battle, const DamageAttackInfo & info) const override;

private:
	const LuaScriptInstance * script;

	std::shared_ptr<LuaContext> contextOf(const CBattleInfoCallback & battle) const;
};

}
