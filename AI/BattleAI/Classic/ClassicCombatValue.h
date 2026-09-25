/*
 * ClassicCombatValue.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <cstdint>
#include <map>
#include "../../../lib/battle/BattleSide.h"

class CBattleInfoCallback;
class CStack;

struct ClassicCombatParameters
{
	int32_t lowestAttack = 0;
	int32_t lowestDefense = 0;
	bool killsOnly = false;
	bool simulated = false;
	/// Damage assigned by earlier actors in a simulated action sequence.
	std::map<uint32_t, int64_t> expectedDamage;
	int64_t friendlyCombatValue = 0;
	int64_t enemyCombatValue = 0;
	int64_t awakeFriendlyValue = 0;
	int64_t awakeEnemyValue = 0;
	int32_t roundsLeft = 1;
	BattleSide ourSide = BattleSide::NONE;
	BattleSide enemySide = BattleSide::NONE;
};

/// Integer combat-value model used by every classic tactical evaluator.
class ClassicCombatValue
{
	std::shared_ptr<CBattleInfoCallback> battle;

public:
	double unitValueExact(
		const CStack * stack,
		const ClassicCombatParameters & parameters,
		bool ranged) const;

	explicit ClassicCombatValue(std::shared_ptr<CBattleInfoCallback> battle);

	ClassicCombatParameters buildParameters(BattleSide side, int32_t difficulty) const;
	int64_t unitValue(const CStack * stack, const ClassicCombatParameters & parameters) const;
	int64_t stackValue(const CStack * stack, const ClassicCombatParameters & parameters) const;
	int64_t stackValue(const CStack * stack, const ClassicCombatParameters & parameters, bool ranged) const;
	int64_t stackValueAtHealth(const CStack * stack, int64_t health, const ClassicCombatParameters & parameters) const;
	int64_t stackValueAtHealth(
		const CStack * stack,
		int64_t health,
		const ClassicCombatParameters & parameters,
		bool ranged) const;
	int64_t lossValue(
		const CStack * stack,
		int64_t healthBefore,
		int64_t healthAfter,
		const ClassicCombatParameters & parameters) const;
	int64_t lossValue(
		const CStack * stack,
		int64_t healthBefore,
		int64_t healthAfter,
		const ClassicCombatParameters & parameters,
		bool ranged,
		bool killsOnly) const;
	int64_t sideValue(BattleSide side, const ClassicCombatParameters & parameters, bool includeCripples) const;

	/// Mirrors the original MSVC x87 integer conversion.
	/// Conversion is truncation toward zero, irrespective of the ambient
	/// floating-point rounding mode.
	static int64_t truncateTowardZero(double value);
};
