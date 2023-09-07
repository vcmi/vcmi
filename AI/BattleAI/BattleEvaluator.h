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
#include "../../lib/AI_Base.h"
#include "../../lib/battle/ReachabilityInfo.h"
#include "PossibleSpellcast.h"
#include "PotentialTargets.h"
#include "BattleExchangeVariant.h"

VCMI_LIB_NAMESPACE_BEGIN

class CSpell;

VCMI_LIB_NAMESPACE_END

class EnemyInfo;

class BattleEvaluator
{
	std::unique_ptr<PotentialTargets> targets;
	std::shared_ptr<HypotheticBattle> hb;
	BattleExchangeEvaluator scoreEvaluator;
	std::shared_ptr<CBattleCallback> cb;
	std::shared_ptr<Environment> env;
	bool activeActionMade = false;
	std::optional<AttackPossibility> cachedAttack;
	PlayerColor playerID;
	BattleID battleID;
	int side;
	float cachedScore;
	DamageCache damageCache;
	float strengthRatio;

public:
	BattleAction selectStackAction(const CStack * stack);
	bool attemptCastingSpell(const CStack * stack);
	bool canCastSpell();
	std::optional<PossibleSpellcast> findBestCreatureSpell(const CStack * stack);
	BattleAction goTowardsNearest(const CStack * stack, std::vector<BattleHex> hexes);
	std::vector<BattleHex> getBrokenWallMoatHexes() const;
	void evaluateCreatureSpellcast(const CStack * stack, PossibleSpellcast & ps); //for offensive damaging spells only
	void print(const std::string & text) const;

	BattleEvaluator(
		std::shared_ptr<Environment> env,
		std::shared_ptr<CBattleCallback> cb,
		const battle::Unit * activeStack,
		PlayerColor playerID,
		BattleID battleID,
		int side,
		float strengthRatio)
		:scoreEvaluator(cb->getBattle(battleID), env, strengthRatio), cachedAttack(), playerID(playerID), side(side), env(env), cb(cb), strengthRatio(strengthRatio), battleID(battleID)
	{
		hb = std::make_shared<HypotheticBattle>(env.get(), cb->getBattle(battleID));
		damageCache.buildDamageCache(hb, side);

		targets = std::make_unique<PotentialTargets>(activeStack, damageCache, hb);
		cachedScore = EvaluationResult::INEFFECTIVE_SCORE;
	}

	BattleEvaluator(
		std::shared_ptr<Environment> env,
		std::shared_ptr<CBattleCallback> cb,
		std::shared_ptr<HypotheticBattle> hb,
		DamageCache & damageCache,
		const battle::Unit * activeStack,
		PlayerColor playerID,
		BattleID battleID,
		int side,
		float strengthRatio)
		:scoreEvaluator(cb->getBattle(battleID), env, strengthRatio), cachedAttack(), playerID(playerID), side(side), env(env), cb(cb), hb(hb), damageCache(damageCache), strengthRatio(strengthRatio), battleID(battleID)
	{
		targets = std::make_unique<PotentialTargets>(activeStack, damageCache, hb);
		cachedScore = EvaluationResult::INEFFECTIVE_SCORE;
	}
};
