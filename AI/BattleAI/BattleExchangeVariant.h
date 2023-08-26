/*
 * BattleExchangeVariant.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/AI_Base.h"
#include "../../lib/battle/ReachabilityInfo.h"
#include "PotentialTargets.h"
#include "StackWithBonuses.h"

struct AttackerValue
{
	float value;
	bool isRetalitated;
	BattleHex position;

	AttackerValue();
};

struct MoveTarget
{
	float score;
	float scorePerTurn;
	std::vector<BattleHex> positions;
	std::optional<AttackPossibility> cachedAttack;
	uint8_t turnsToRich;

	MoveTarget();
};

struct EvaluationResult
{
	static const int64_t INEFFECTIVE_SCORE = -10000;

	AttackPossibility bestAttack;
	MoveTarget bestMove;
	bool wait;
	float score;
	bool defend;

	EvaluationResult(const AttackPossibility & ap)
		:wait(false), score(INEFFECTIVE_SCORE), bestAttack(ap), defend(false)
	{
	}
};

/// <summary>
/// The class represents evaluation of attack value
/// of exchanges between all stacks which can access particular hex
/// starting from initial attack represented by AttackPossibility and further according turn order.
/// Negative score value means we get more demage than deal
/// </summary>
class BattleExchangeVariant
{
public:
	BattleExchangeVariant(float positiveEffectMultiplier, float negativeEffectMultiplier)
		: dpsScore(0), positiveEffectMultiplier(positiveEffectMultiplier), negativeEffectMultiplier(negativeEffectMultiplier) {}

	float trackAttack(
		const AttackPossibility & ap,
		std::shared_ptr<HypotheticBattle> hb,
		DamageCache & damageCache);

	float trackAttack(
		std::shared_ptr<StackWithBonuses> attacker,
		std::shared_ptr<StackWithBonuses> defender,
		bool shooting,
		bool isOurAttack,
		DamageCache & damageCache,
		std::shared_ptr<HypotheticBattle> hb,
		bool evaluateOnly = false);

	float getScore() const { return dpsScore; }

	void adjustPositions(
		std::vector<const battle::Unit *> attackers,
		const AttackPossibility & ap,
		std::map<BattleHex, battle::Units> & reachabilityMap);

private:
	float positiveEffectMultiplier;
	float negativeEffectMultiplier;
	float dpsScore;
	std::map<uint32_t, AttackerValue> attackerValue;
};

class BattleExchangeEvaluator
{
private:
	std::shared_ptr<CBattleInfoCallback> cb;
	std::shared_ptr<Environment> env;
	std::map<BattleHex, std::vector<const battle::Unit *>> reachabilityMap;
	std::vector<battle::Units> turnOrder;
	float negativeEffectMultiplier;

public:
	BattleExchangeEvaluator(
		std::shared_ptr<CBattleInfoCallback> cb,
		std::shared_ptr<Environment> env,
		float strengthRatio): cb(cb), env(env) {
		negativeEffectMultiplier = strengthRatio;
	}

	EvaluationResult findBestTarget(
		const battle::Unit * activeStack,
		PotentialTargets & targets,
		DamageCache & damageCache,
		std::shared_ptr<HypotheticBattle> hb);

	float calculateExchange(
		const AttackPossibility & ap,
		PotentialTargets & targets,
		DamageCache & damageCache,
		std::shared_ptr<HypotheticBattle> hb);

	void updateReachabilityMap(std::shared_ptr<HypotheticBattle> hb);
	std::vector<const battle::Unit *> getExchangeUnits(const AttackPossibility & ap, PotentialTargets & targets, std::shared_ptr<HypotheticBattle> hb);
	bool checkPositionBlocksOurStacks(HypotheticBattle & hb, const battle::Unit * unit, BattleHex position);

	MoveTarget findMoveTowardsUnreachable(
		const battle::Unit * activeStack,
		PotentialTargets & targets,
		DamageCache & damageCache,
		std::shared_ptr<HypotheticBattle> hb);

	std::vector<const battle::Unit *> getAdjacentUnits(const battle::Unit * unit);

	float getPositiveEffectMultiplier() { return 1; }
	float getNegativeEffectMultiplier() { return negativeEffectMultiplier; }
};