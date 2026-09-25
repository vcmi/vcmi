/*
 * ClassicAttackEvaluator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/battle/IBattleInfoCallback.h"
#include "ClassicCombatValue.h"

class ClassicBattleStateView;
class ClassicDecisionTrace;
class CBattleInfoCallback;
class CStack;
class IClassicBattleAIRng;
struct ReachabilityInfo;

namespace battle
{
	class Unit;
}

struct ClassicScoredAction
{
	BattleAction action;
	int64_t score = std::numeric_limits<int64_t>::min();
	int32_t attackTime = std::numeric_limits<int32_t>::max();
	bool valid = false;
};

using ClassicExpectedDamage = std::map<uint32_t, int64_t>;

struct ClassicProjectedTarget
{
	const CStack * target = nullptr;
	int64_t value = 0;
	int32_t distance = 0;
	int32_t attackTime = 0;
	uint32_t possibleTargets = 0;
};

using ClassicProjectedTargets = std::map<uint32_t, ClassicProjectedTarget>;

struct ClassicProjectedExchange
{
	int64_t attackerLoss = 0;
	int64_t defenderLoss = 0;
};

/// Original-style shooter, melee, movement, war-machine, and defensive selection.
class ClassicAttackEvaluator
{
	using DangerMap = std::array<int32_t, GameConstants::BFIELD_SIZE>;
	enum class RetaliationMode
	{
		NORMAL,
		NEVER,
		ALWAYS
	};

	struct SimulatedAttack
	{
		int64_t attackerBefore = 0;
		int64_t defenderBefore = 0;
		int64_t attackerAfter = 0;
		int64_t defenderAfter = 0;
		int64_t firstStrike = 0;
		int64_t firstFireShield = 0;
		int64_t retaliation = 0;
		int64_t retaliationFireShield = 0;
		int64_t secondStrike = 0;
		int64_t secondFireShield = 0;
	};

	std::shared_ptr<CBattleInfoCallback> battle;
	std::shared_ptr<IClassicBattleAIRng> randomGenerator;
	std::shared_ptr<ClassicDecisionTrace> trace;
	ClassicCombatValue combatValue;
	bool preserveLongMoveWait;
	bool secondPhase;

	int64_t averageDamage(
		const CBattleInfoCallback & scenario,
		const battle::Unit * attacker,
		int64_t attackerHealth,
		const battle::Unit * defender,
		int64_t defenderHealth,
		const BattleHex & attackerPosition,
		bool ranged,
		int32_t distance) const;
	int64_t averageDamage(
		const CStack * attacker,
		int64_t attackerHealth,
		const CStack * defender,
		int64_t defenderHealth,
		const BattleHex & attackerPosition,
		bool ranged,
		int32_t distance) const;
	int64_t fireShieldDamage(
		const CBattleInfoCallback & scenario,
		const battle::Unit * shieldBearer,
		int64_t shieldBearerHealth,
		const battle::Unit * recipient,
		int64_t incomingDamage) const;
	SimulatedAttack simulateAttack(
		const CBattleInfoCallback & scenario,
		const battle::Unit * attacker,
		const battle::Unit * defender,
		const BattleHex & attackFrom,
		bool ranged,
		int32_t distance,
		int64_t attackerHealth = -1,
		int64_t defenderHealth = -1,
		RetaliationMode retaliationMode = RetaliationMode::NORMAL) const;
	SimulatedAttack simulateAttack(
		const CStack * attacker,
		const CStack * defender,
		const BattleHex & attackFrom,
		bool ranged,
		int32_t distance,
		int64_t attackerHealth = -1,
		int64_t defenderHealth = -1) const;
	ClassicProjectedTargets findProjectedTargets(
		BattleSide actorPhysicalSide,
		const ClassicCombatParameters & parameters,
		const CStack * excluded = nullptr,
		bool onlyPriorTargets = false) const;
	int64_t rangedTargetValue(
		const CStack * attacker,
		const CStack * target,
		const ClassicCombatParameters & parameters,
		const ClassicProjectedTargets & projection,
		const std::string * areaTraceKey = nullptr) const;
	int64_t areaShotValue(
		const CStack * attacker,
		const BattleHex & center,
		const ClassicCombatParameters & parameters,
		const ClassicProjectedTargets & projection) const;

	int64_t attackValue(
		const CStack * attacker,
		const CStack * defender,
		const BattleHex & attackFrom,
		bool ranged,
		const ClassicCombatParameters & parameters,
		int32_t distanceOverride = -1,
		const std::string * areaTraceKey = nullptr) const;
	int64_t attackChange(
		const CStack * attacker,
		const CStack * target,
		const ClassicCombatParameters & parameters,
		const ClassicProjectedTargets & projection) const;
	int64_t landingContextValue(
		const CStack * attacker,
		const BattleHex & landing,
		const ClassicCombatParameters & parameters) const;
	int64_t meleeCollateralValue(
		const CStack * attacker,
		const CStack * primaryTarget,
		const BattleHex & landing,
		const ClassicCombatParameters & parameters) const;
	DangerMap buildDangerMap(
		const CStack * stack,
		const ClassicCombatParameters & parameters) const;
	static int32_t dangerAt(
		const CStack * stack,
		const BattleHex & head,
		const DangerMap & dangerMap);
	ClassicScoredAction chooseShooterAction(const CStack * stack, const ClassicCombatParameters & parameters) const;
	ClassicScoredAction chooseMeleeAction(const CStack * stack, const ClassicCombatParameters & parameters) const;
	bool pathCrossesClosedGate(
		const CStack * stack,
		const BattleHex & destination,
		const ReachabilityInfo & reachability) const;
	ClassicScoredAction chooseShooterDefense(const CStack * stack, const ClassicCombatParameters & parameters) const;
	ClassicScoredAction chooseRunAction(const CStack * stack, const DangerMap & dangerMap) const;
	ClassicScoredAction chooseBerserkAction(const CStack * stack) const;
	BattleAction moveToward(
		const CStack * stack,
		const BattleHex & target,
		bool mayWait,
		const DangerMap * dangerMap = nullptr) const;
	BattleHex siegeAdvanceTarget(const CStack * stack) const;
	BattleAction chooseCyclopsAction(const CStack * stack) const;
	int64_t strandedFriendlyValue(const CStack * stack, const ClassicCombatParameters & parameters) const;

public:
	ClassicAttackEvaluator(
		std::shared_ptr<CBattleInfoCallback> battle,
		std::shared_ptr<IClassicBattleAIRng> randomGenerator,
		std::shared_ptr<ClassicDecisionTrace> trace,
		bool preserveLongMoveWait = false,
		bool secondPhase = false
	);

	ClassicScoredAction chooseAction(const CStack * stack, const ClassicCombatParameters & parameters) const;
	/// Recovered choose_to_run test seam. Production calls the same selector
	/// only after ordinary, siege, spell, and shooter-defense paths fail.
	ClassicScoredAction chooseRunAction(const CStack * stack, const ClassicCombatParameters & parameters) const;
	BattleAction chooseTacticsShooterPlacement(const CStack * stack, bool enabled) const;
	ClassicExpectedDamage projectExpectedDamage(
		BattleSide side,
		bool includeOtherSide,
		const ClassicCombatParameters & parameters) const;
	ClassicProjectedTargets projectSpellTargets(
		BattleSide physicalSide,
		const ClassicCombatParameters & parameters) const;
	int64_t spellExchangeEffect(
		const CStack * first,
		const CStack * second,
		const ClassicCombatParameters & parameters) const;
	/// Scores one projected exchange in an alternate battle state while retaining
	/// the original stacks' classic combat-value and expected-health accounting.
	int64_t projectedExchangeValue(
		const CBattleInfoCallback & scenario,
		const battle::Unit * attacker,
		const battle::Unit * defender,
		const CStack * originalAttacker,
		const CStack * originalDefender,
		const BattleHex & attackFrom,
		bool ranged,
		int32_t distance,
		const ClassicCombatParameters & parameters) const;
	/// Returns each side of the exchange separately for effects such as Berserk,
	/// where attacker and defender can belong to the same physical army.
	ClassicProjectedExchange projectedExchangeLosses(
		const CBattleInfoCallback & scenario,
		const battle::Unit * attacker,
		const battle::Unit * defender,
		const CStack * originalAttacker,
		const CStack * originalDefender,
		const BattleHex & attackFrom,
		bool ranged,
		int32_t distance,
		const ClassicCombatParameters & parameters) const;
	/// Returns the defender-side benefit of one otherwise legal retaliation.
	int64_t projectedRetaliationValue(
		const CBattleInfoCallback & scenario,
		const battle::Unit * attacker,
		const battle::Unit * defender,
		const CStack * originalAttacker,
		const CStack * originalDefender,
		const BattleHex & attackFrom,
		int32_t distance,
		const ClassicCombatParameters & parameters) const;
	static bool shouldReplaceShooterTarget(
		int64_t candidateScore,
		bool candidateDisabled,
		int64_t bestScore,
		bool bestDisabled);
	BattleAction chooseCatapultAction(const CStack * stack) const;
	BattleAction chooseHealingTentAction(const CStack * stack) const;
};
