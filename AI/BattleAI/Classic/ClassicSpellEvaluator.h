/*
 * ClassicSpellEvaluator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../../lib/battle/BattleAction.h"
#include "ClassicCombatValue.h"

class CBattleInfoCallback;
class CStack;
class ClassicAttackEvaluator;
class ClassicDecisionTrace;
class Environment;
class HypotheticBattle;
class IClassicBattleAIRng;

struct ClassicScoredSpell
{
	BattleAction action;
	int64_t rawValue = 0;
	int64_t score = 0;
	bool valid = false;
};

/// Sequential spell-ID/target evaluator with original mana and random score scaling.
class ClassicSpellEvaluator
{
	const Environment * env;
	std::shared_ptr<CBattleInfoCallback> battle;
	std::shared_ptr<IClassicBattleAIRng> randomGenerator;
	std::shared_ptr<ClassicDecisionTrace> trace;
	const ClassicAttackEvaluator * attackEvaluator;

	int64_t evaluateStateChange(const HypotheticBattle & after, BattleSide side) const;
	int64_t manaAdjustedValue(int64_t rawValue, int32_t currentMana, int32_t cost) const;
	ClassicScoredSpell chooseRandomBeneficialSpell(const CStack * caster, int64_t competingValue) const;

public:
	ClassicSpellEvaluator(
		const Environment * env,
		std::shared_ptr<CBattleInfoCallback> battle,
		std::shared_ptr<IClassicBattleAIRng> randomGenerator,
		std::shared_ptr<ClassicDecisionTrace> trace,
		const ClassicAttackEvaluator * attackEvaluator = nullptr
	);

	ClassicScoredSpell chooseHeroSpell(
		BattleSide side,
		bool retreating,
		const ClassicCombatParameters & parameters) const;
	ClassicScoredSpell chooseCreatureSpell(
		const CStack * caster,
		int64_t competingValue,
		const ClassicCombatParameters & parameters) const;
	bool canChooseHeroSpell(BattleSide side) const;

	static int64_t applyManaConservation(int64_t value, int32_t castsAvailable);
	static int32_t classicSlowSpeed(int32_t currentEffectiveSpeed, int32_t effectLevel);
};
