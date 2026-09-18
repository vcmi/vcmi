/*
 * ClassicRetreatEvaluator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ClassicCombatValue.h"

class CGHeroInstance;
class ClassicDecisionTrace;
class CBattleInfoCallback;
class IClassicBattleAIRng;
class Environment;

/// Implements the original hard retreat gates and projected-strength threshold.
class ClassicRetreatEvaluator
{
	std::shared_ptr<CBattleInfoCallback> battle;
	std::shared_ptr<IClassicBattleAIRng> randomGenerator;
	std::shared_ptr<ClassicDecisionTrace> trace;
	const Environment * env;

	int64_t artifactValue(const CGHeroInstance * hero) const;
	int64_t projectedStrength(BattleSide side) const;
	double retreatThreshold(BattleSide side, int32_t difficulty, int64_t artifacts, int64_t experience) const;

public:
	ClassicRetreatEvaluator(
		std::shared_ptr<CBattleInfoCallback> battle,
		std::shared_ptr<IClassicBattleAIRng> randomGenerator,
		std::shared_ptr<ClassicDecisionTrace> trace,
		const Environment * env = nullptr
	);

	bool shouldRetreat(
		BattleSide side,
		int32_t difficulty,
		const ClassicCombatParameters & parameters,
		const std::map<uint32_t, int64_t> * precomputedDamage = nullptr) const;

	static int64_t applyTenPercentBias(int64_t value);
};
