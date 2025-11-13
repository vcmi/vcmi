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
	bool isRetaliated;
	BattleHex position;

	AttackerValue();
};

struct MoveTarget
{
	float score;
	BattleHexArray positions;
	std::optional<AttackPossibility> cachedAttack;
	uint8_t turnsToReach;

	MoveTarget();
};

struct EvaluationResult
{
	static const int64_t INEFFECTIVE_SCORE = -100000000;

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
	std::map<int, battle::Units> units;

	// shooters which are within mellee attack and mellee units
	battle::Units melleeAccessible;

	// far shooters
	battle::Units shooters;

	std::set<uint32_t> enemyUnitsReachingAttacker;
};

class ReachabilityMapCache
{
	struct PerTurnData{
		std::bitset<GameConstants::BFIELD_SIZE> isValid;
		std::array<battle::Units, GameConstants::BFIELD_SIZE> hexes;
	};

	std::map<uint32_t, ReachabilityInfo> unitReachabilityMap; // unit ID -> reachability
	std::map<uint32_t, PerTurnData> hexReachabilityPerTurn;

	battle::Units computeOneTurnReachableUnits(std::shared_ptr<CBattleInfoCallback> cb, std::shared_ptr<Environment> env, const std::vector<battle::Units> & turnOrder, uint8_t turn, const BattleHex & hex);
public:
	const battle::Units & getOneTurnReachableUnits(std::shared_ptr<CBattleInfoCallback> cb, std::shared_ptr<Environment> env, const std::vector<battle::Units> & turnOrder, uint8_t turn, const BattleHex & hex);
	void update(const std::vector<battle::Units> & turnOrder, std::shared_ptr<HypotheticBattle> hb);
};

class BattleExchangeEvaluator
{
private:
	std::shared_ptr<CBattleInfoCallback> cb;
	std::shared_ptr<Environment> env;
	mutable ReachabilityMapCache reachabilityMap;
	std::vector<battle::Units> turnOrder;
	float negativeEffectMultiplier;
	int simulationTurnsCount;

	float scoreValue(const BattleScore & score) const;

	BattleScore calculateExchange(
		const AttackPossibility & ap,
		uint8_t turn,
		PotentialTargets & targets,
		DamageCache & damageCache,
		std::shared_ptr<HypotheticBattle> hb,
		const battle::Units & additionalUnits = {}) const;

	bool canBeHitThisTurn(const AttackPossibility & ap);

public:
	BattleExchangeEvaluator(
		std::shared_ptr<CBattleInfoCallback> cb,
		std::shared_ptr<Environment> env,
		float strengthRatio,
		int simulationTurnsCount): cb(cb), env(env), simulationTurnsCount(simulationTurnsCount){
		negativeEffectMultiplier = strengthRatio >= 1 ? 1 : strengthRatio * strengthRatio;
	}

	EvaluationResult findBestTarget(
		const battle::Unit * activeStack,
		PotentialTargets & targets,
		DamageCache & damageCache,
		std::shared_ptr<HypotheticBattle> hb,
		bool siegeDefense = false);

	float evaluateExchange(
		const AttackPossibility & ap,
		uint8_t turn,
		PotentialTargets & targets,
		DamageCache & damageCache,
		std::shared_ptr<HypotheticBattle> hb) const;

	const battle::Units & getOneTurnReachableUnits(uint8_t turn, const BattleHex & hex) const;
	void updateReachabilityMap(std::shared_ptr<HypotheticBattle> hb);

	ReachabilityData getExchangeUnits(
		const AttackPossibility & ap,
		uint8_t turn,
		PotentialTargets & targets,
		std::shared_ptr<HypotheticBattle> hb,
		const battle::Units & additionalUnits = {}) const;

	bool checkPositionBlocksOurStacks(const HypotheticBattle & hb, const battle::Unit * unit, const BattleHex & position);

	MoveTarget findMoveTowardsUnreachable(
		const battle::Unit * activeStack,
		PotentialTargets & targets,
		DamageCache & damageCache,
		std::shared_ptr<HypotheticBattle> hb);

	battle::Units getAdjacentUnits(const battle::Unit * unit) const;

	float getPositiveEffectMultiplier() const { return 1; }
	float getNegativeEffectMultiplier() const { return negativeEffectMultiplier; }
};
