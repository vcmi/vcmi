/*
 * BattleEvaluator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../../lib/battle/ReachabilityInfo.h"
#include "PossibleSpellcast.h"
#include "PotentialTargets.h"
#include "BattleExchangeVariant.h"

VCMI_LIB_NAMESPACE_BEGIN

class CSpell;
class CBattleCallback;
class BattleAction;

VCMI_LIB_NAMESPACE_END

class EnemyInfo;

struct CachedAttack
{
	std::optional<AttackPossibility> ap;
	float score = EvaluationResult::INEFFECTIVE_SCORE;
	uint8_t turn = 255;
	bool waited = false;
};

class BattleEvaluator
{
	std::unique_ptr<PotentialTargets> targets;
	std::shared_ptr<HypotheticBattle> hb;
	BattleExchangeEvaluator scoreEvaluator;
	std::shared_ptr<CBattleCallback> cb;
	std::shared_ptr<Environment> env;
	bool activeActionMade = false;
	CachedAttack cachedAttack;
	PlayerColor playerID;
	BattleID battleID;
	BattleSide side;
	DamageCache damageCache;
	float strengthRatio;
	int simulationTurnsCount;

public:
	BattleAction selectStackAction(const CStack * stack);
	bool attemptCastingSpell(const CStack * stack);
	bool canCastSpell();
	std::optional<PossibleSpellcast> findBestCreatureSpell(const CStack * stack);
	BattleAction goTowardsNearest(const CStack * stack, const BattleHexArray & hexes, const PotentialTargets & targets);
	std::vector<BattleHex> getBrokenWallMoatHexes() const;
	bool hasWorkingTowers() const;
	void evaluateCreatureSpellcast(const CStack * stack, PossibleSpellcast & ps); //for offensive damaging spells only
	void print(const std::string & text) const;
	BattleAction moveOrAttack(const CStack * stack, const BattleHex & hex, const PotentialTargets & targets);

	BattleEvaluator(
		std::shared_ptr<Environment> env,
		std::shared_ptr<CBattleCallback> cb,
		const battle::Unit * activeStack,
		PlayerColor playerID,
		BattleID battleID,
		BattleSide side,
		float strengthRatio,
		int simulationTurnsCount);

	BattleEvaluator(
		std::shared_ptr<Environment> env,
		std::shared_ptr<CBattleCallback> cb,
		std::shared_ptr<HypotheticBattle> hb,
		DamageCache & damageCache,
		const battle::Unit * activeStack,
		PlayerColor playerID,
		BattleID battleID,
		BattleSide side,
		float strengthRatio,
		int simulationTurnsCount);
};
