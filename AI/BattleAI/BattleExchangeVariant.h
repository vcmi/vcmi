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

struct BattleScore
{
	float ourDamageReduce;
	float enemyDamageReduce;

	BattleScore(float enemyDamageReduce, float ourDamageReduce)
		:enemyDamageReduce(enemyDamageReduce), ourDamageReduce(ourDamageReduce)
	{
	}

	BattleScore() : BattleScore(0, 0) {}

	float value()
	{
		return enemyDamageReduce - ourDamageReduce;
	}

	BattleScore  operator+(BattleScore & other)
	{
		BattleScore result = *this;

		result.ourDamageReduce += other.ourDamageReduce;
		result.enemyDamageReduce += other.enemyDamageReduce;

		return result;
	}
};

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
	BattleExchangeVariant()
		: dpsScore() {}

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

	const BattleScore & getScore() const { return dpsScore; }

private:
	BattleScore dpsScore;
	std::map<uint32_t, AttackerValue> attackerValue;
};

struct ReachabilityData
{
	std::vector<const battle::Unit *> units;

	// shooters which are within mellee attack and mellee units
	std::vector<const battle::Unit *> melleeAccessible;

	// far shooters
	std::vector<const battle::Unit *> shooters;
};

class BattleExchangeEvaluator
{
private:
	std::shared_ptr<CBattleInfoCallback> cb;
	std::shared_ptr<Environment> env;
	std::map<uint32_t, ReachabilityInfo> reachabilityCache;
	std::map<BattleHex, std::vector<const battle::Unit *>> reachabilityMap;
	std::vector<battle::Units> turnOrder;
	float negativeEffectMultiplier;

	float scoreValue(const BattleScore & score) const;

	BattleScore calculateExchange(
		const AttackPossibility & ap,
		uint8_t turn,
		PotentialTargets & targets,
		DamageCache & damageCache,
		std::shared_ptr<HypotheticBattle> hb);

	bool canBeHitThisTurn(const AttackPossibility & ap);

public:
	BattleExchangeEvaluator(
		std::shared_ptr<CBattleInfoCallback> cb,
		std::shared_ptr<Environment> env,
		float strengthRatio): cb(cb), env(env) {
		negativeEffectMultiplier = strengthRatio >= 1 ? 1 : strengthRatio;
	}

	EvaluationResult findBestTarget(
		const battle::Unit * activeStack,
		PotentialTargets & targets,
		DamageCache & damageCache,
		std::shared_ptr<HypotheticBattle> hb);

	float evaluateExchange(
		const AttackPossibility & ap,
		uint8_t turn,
		PotentialTargets & targets,
		DamageCache & damageCache,
		std::shared_ptr<HypotheticBattle> hb);

	std::vector<const battle::Unit *> getOneTurnReachableUnits(uint8_t turn, BattleHex hex);
	void updateReachabilityMap(std::shared_ptr<HypotheticBattle> hb);

	ReachabilityData getExchangeUnits(
		const AttackPossibility & ap,
		uint8_t turn,
		PotentialTargets & targets,
		std::shared_ptr<HypotheticBattle> hb);

	bool checkPositionBlocksOurStacks(HypotheticBattle & hb, const battle::Unit * unit, BattleHex position);

	MoveTarget findMoveTowardsUnreachable(
		const battle::Unit * activeStack,
		PotentialTargets & targets,
		DamageCache & damageCache,
		std::shared_ptr<HypotheticBattle> hb);

	std::vector<const battle::Unit *> getAdjacentUnits(const battle::Unit * unit) const;

	float getPositiveEffectMultiplier() const { return 1; }
	float getNegativeEffectMultiplier() const { return negativeEffectMultiplier; }
};
