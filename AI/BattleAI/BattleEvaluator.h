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

class CSpell;
class CBattleCallback;
class BattleAction;

struct CachedAttack
{
	std::optional<AttackPossibility> ap;
	float score = EvaluationResult::INEFFECTIVE_SCORE;
	uint8_t turn = 255;
	bool waited = false;
};

class DLL_EXPORT BattleEvaluator
{
	std::unique_ptr<PotentialTargets> targets;
	std::shared_ptr<HypotheticBattle> hb;
	BattleExchangeEvaluator scoreEvaluator;
	std::shared_ptr<CBattleCallback> cb;
	std::shared_ptr<Environment> env;
	bool activeActionMade = false;
	std::atomic<bool> breachCastFound = false;
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
	BattleAction goTowardsNearest(
		const CStack * stack,
		const BattleHexArray & movementTargets,
		const PotentialTargets & potentialTargets,
		const BattleHexArray & finalDestinationHexes);
	std::vector<BattleHex> getBrokenWallMoatHexes() const;
	bool hasWorkingTowers() const;
	/// Whether town walls or gate already have a passage for attacking units
	static bool wallsBreached(const CBattleInfoCallback & battle);
	/// Value of the first damage to unbreached walls, as share of enemy army that our walking units could reach
	float breachValue() const;
	/// Whether last attemptCastingSpell found a cast that opens walls our walking units need opened
	bool canBreachWalls() const { return breachCastFound; }
	void evaluateCreatureSpellcast(const CStack * stack, PossibleSpellcast & ps);
	/// Value that bonuses of a simulated cast add to our army and take away from the enemy. Health that
	/// the same cast changed is scored separately, so only what the bonuses are worth is counted here
	float scoreBonusEffects(const battle::Units & units, const DamageCache & cache) const;
	void print(const std::string & text) const;
	BattleAction moveOrAttack(
		const CStack * stack,
		const BattleHex & movementTarget,
		const PotentialTargets & potentialTargets,
		const BattleHexArray & allowedAttackOrigins = {});

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
