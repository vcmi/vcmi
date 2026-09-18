/*
 * ClassicBattleDecision.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../../lib/battle/AutocombatPreferences.h"
#include "../../../lib/battle/BattleAction.h"

class CBattleInfoCallback;
class CStack;
class ClassicDecisionTrace;
class Environment;
class IClassicBattleAIRng;

enum class ClassicDecisionEntryPoint
{
	FULL_PIPELINE,
	DO_SPELL_AI,
	DO_COMP_AI
};

/// Evaluates the classic decision pipeline without submitting actions to the server.
class ClassicBattleDecision
{
public:
	static bool supportsBattle(const CBattleInfoCallback & battle);

	static BattleAction decide(
		const Environment * env,
		std::shared_ptr<CBattleInfoCallback> battle,
		BattleSide side,
		const CStack * stack,
		int32_t difficulty,
		const AutocombatPreferences & preferences,
		std::shared_ptr<IClassicBattleAIRng> randomGenerator,
		std::shared_ptr<ClassicDecisionTrace> trace,
		ClassicDecisionEntryPoint entryPoint = ClassicDecisionEntryPoint::FULL_PIPELINE
	);
};
