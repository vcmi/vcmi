/*
 * ClassicRetreatEvaluator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"
#include "ClassicRetreatEvaluator.h"

#include "../../../lib/CStack.h"
#include "../../../lib/CPlayerState.h"
#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/callback/IGameInfoCallback.h"
#include "../../../lib/entities/artifact/ArtSlotInfo.h"
#include "../../../lib/entities/artifact/CArtifact.h"
#include "../../../lib/entities/artifact/CArtifactInstance.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "ClassicBattleRng.h"
#include "ClassicAttackEvaluator.h"
#include "ClassicBattleStateView.h"
#include "ClassicDecisionTrace.h"
#include "ClassicRulesAdapter.h"
#include <vcmi/Environment.h>

ClassicRetreatEvaluator::ClassicRetreatEvaluator(
	std::shared_ptr<CBattleInfoCallback> battle,
	std::shared_ptr<IClassicBattleAIRng> randomGenerator,
	std::shared_ptr<ClassicDecisionTrace> trace,
	const Environment * env
)
	: battle(std::move(battle)), randomGenerator(std::move(randomGenerator)), trace(std::move(trace)), env(env)
{
}

int64_t ClassicRetreatEvaluator::artifactValue(const CGHeroInstance * hero) const
{
	int64_t result = 0;
	for(const auto & [position, slot] : hero->artifactsWorn)
	{
		const auto * artifact = slot.getArt();
		if(artifact)
		{
			// AICheckRetreat uses the larger of the context-sensitive artifact
			// evaluator and half the base artifact cost. The evaluator itself has
			// a floor of ten; price/2 is the exact lower bound available from the
			// VCMI artifact model for artifacts without an extra payload.
			result += std::max<int64_t>(10, artifact->getType()->getPrice() / 2);
		}
	}
	for(const ArtSlotInfo & slot : hero->artifactsInBackpack)
	{
		const auto * artifact = slot.getArt();
		if(artifact)
			result += std::max<int64_t>(10, artifact->getType()->getPrice() / 2);
	}
	return result;
}

int64_t ClassicRetreatEvaluator::projectedStrength(BattleSide side) const
{
	int64_t result = 0;
	ClassicBattleStateView view(battle);
	for(const CStack * stack : view.orderedStacks(false, true))
	{
		if(stack->unitSide() != side || !stack->alive())
			continue;
		const int64_t raw = static_cast<int64_t>(stack->getCount())
			* stack->unitType()->getFightValue();
		result += stack->isClone() ? raw : ClassicCombatValue::truncateTowardZero(raw * 1.2);
	}
	return result;
}

double ClassicRetreatEvaluator::retreatThreshold(
	BattleSide side,
	int32_t difficulty,
	int64_t artifacts,
	int64_t experience) const
{
	double threshold = artifacts == 0 ? 0.16 : artifacts <= 5000 ? 0.20 : artifacts <= 10000 ? 0.21 : 0.22;
	threshold -= (4 - difficulty) * 0.015;
	threshold += std::min(static_cast<double>(experience / 200000), 0.03);
	if(side == BattleSide::ATTACKER)
		threshold -= 0.06;
	return std::min(threshold, 0.16);
}

int64_t ClassicRetreatEvaluator::applyTenPercentBias(int64_t value)
{
	return static_cast<int64_t>(std::llround(1.10 * value));
}

bool ClassicRetreatEvaluator::shouldRetreat(
	BattleSide side,
	int32_t difficulty,
	const ClassicCombatParameters & parameters,
	const std::map<uint32_t, int64_t> * precomputedDamage) const
{
	if(side == BattleSide::NONE || !battle->battleCanFlee(battle->sideToPlayer(side)))
		return false;
	const CGHeroInstance * hero = battle->battleGetFightingHero(side);
	if(!hero || hero->patrol.patrolling || battle->battleTacticDist())
		return false;
	const PlayerColor player = battle->sideToPlayer(side);
	const PlayerState * playerState = env && player.isValidPlayer()
		? env->game()->getPlayerState(player, false)
		: nullptr;
	const bool localHumanAutocombat = playerState && playerState->isHuman();
	if(!localHumanAutocombat && difficulty == 0)
		return false;
	if(!localHumanAutocombat && difficulty == 1)
	{
		const int32_t roll = randomGenerator->nextIntInclusive(1, 100);
		if(trace)
			trace->record("retreat", "difficultyRoll", roll);
		if(roll <= 50)
			return false;
	}

	const int64_t artifacts = artifactValue(hero);
	if(trace)
		trace->record("retreat", "artifacts", artifacts);
	if(artifacts <= 999 && hero->exp <= 1999)
		return false;
	if(ClassicRulesAdapter::failedSiege(battle, side))
		return true;

	ClassicExpectedDamage ownedExpectedDamage;
	if(!precomputedDamage)
	{
		ClassicAttackEvaluator projectionEvaluator(battle, randomGenerator, trace);
		ownedExpectedDamage = projectionEvaluator.projectExpectedDamage(side, true, parameters);
		precomputedDamage = &ownedExpectedDamage;
	}
	bool projectedSurvivor = false;
	ClassicBattleStateView projectedView(battle);
	for(const CStack * stack : projectedView.orderedStacks(false, true))
	{
		if(stack->unitSide() != side
		   || stack->hasBonusOfType(BonusType::SIEGE_WEAPON))
		{
			continue;
		}
		const auto found = precomputedDamage->find(stack->unitId());
		const int64_t damage = found == precomputedDamage->end() ? 0 : found->second;
		const int64_t health = stack->isClone() ? 1 : stack->getAvailableHealth();
		if(health - damage > 0)
		{
			projectedSurvivor = true;
			break;
		}
	}
	if(!projectedSurvivor)
		return true;

	if(env && player.isValidPlayer())
	{
		const int32_t surrenderCost = battle->battleGetSurrenderCost(player);
		const int32_t gold = env->game()->getResource(player, GameResID::GOLD);
		if(gold < surrenderCost + 2500)
			return false;
	}

	int64_t ourValue = projectedStrength(side);
	const BattleSide enemySide = side == BattleSide::ATTACKER
		? BattleSide::DEFENDER
		: BattleSide::ATTACKER;
	int64_t enemyValue = projectedStrength(enemySide);
	if(battle->battleGetDefendedTown())
	{
		if(side == BattleSide::DEFENDER)
			ourValue = applyTenPercentBias(ourValue);
		else
			enemyValue = applyTenPercentBias(enemyValue);
	}
	enemyValue = applyTenPercentBias(enemyValue);
	const int64_t total = ourValue + enemyValue;
	if(total <= 0)
		return false;
	const double share = static_cast<double>(ourValue) / total;
	const double threshold = retreatThreshold(side, difficulty, artifacts, hero->exp);
	if(trace)
	{
		trace->record("retreat", "ourValue", ourValue);
		trace->record("retreat", "enemyValue", enemyValue);
		trace->record("retreat", "shareMillionths", static_cast<int64_t>(share * 1000000));
		trace->record("retreat", "thresholdMillionths", static_cast<int64_t>(threshold * 1000000));
	}
	return share < threshold;
}
