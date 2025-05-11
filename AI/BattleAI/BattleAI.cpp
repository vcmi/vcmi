/*
 * BattleAI.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleAI.h"
#include "BattleEvaluator.h"
#include "BattleExchangeVariant.h"

#include "StackWithBonuses.h"
#include "EnemyInfo.h"
#include "tbb/parallel_for.h"
#include "../../lib/CStopWatch.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/callback/CBattleCallback.h"
#include "../../lib/callback/IGameInfoCallback.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/BattleStateInfoForRetreat.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/StartInfo.h"
#include "../../lib/CStack.h" // TODO: remove
                              // Eventually only IBattleInfoCallback and battle::Unit should be used,
                              // CUnitState should be private and CStack should be removed completely
#include "../../lib/logging/VisualLogger.h"

#define LOGL(text) print(text)
#define LOGFL(text, formattingEl) print(boost::str(boost::format(text) % formattingEl))

CBattleAI::CBattleAI()
	: side(BattleSide::NONE),
	wasWaitingForRealize(false)
{
}

CBattleAI::~CBattleAI()
{
	if(cb)
	{
		//Restore previous state of CB - it may be shared with the main AI (like VCAI)
		cb->waitTillRealize = wasWaitingForRealize;
	}
}

void logHexNumbers()
{
#if BATTLE_TRACE_LEVEL >= 1
	logVisual->updateWithLock("hexes", [](IVisualLogBuilder & b)
		{
			for(BattleHex hex = BattleHex(0); hex < GameConstants::BFIELD_SIZE; ++hex)
				b.addText(hex, std::to_string(hex.toInt()));
		});
#endif
}

void CBattleAI::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB)
{
	env = ENV;
	cb = CB;
	playerID = *CB->getPlayerID();
	wasWaitingForRealize = CB->waitTillRealize;
	CB->waitTillRealize = false;
	movesSkippedByDefense = 0;

	logHexNumbers();
}

void CBattleAI::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences autocombatPreferences)
{
	initBattleInterface(ENV, CB);
	autobattlePreferences = autocombatPreferences;
}

BattleAction CBattleAI::useHealingTent(const BattleID & battleID, const CStack *stack)
{
	auto healingTargets = cb->getBattle(battleID)->battleGetStacks(CBattleInfoEssentials::ONLY_MINE);
	std::map<int, const CStack*> woundHpToStack;
	for(const auto * stack : healingTargets)
	{
		if(auto woundHp = stack->getMaxHealth() - stack->getFirstHPleft())
			woundHpToStack[woundHp] = stack;
	}

	if(woundHpToStack.empty())
		return BattleAction::makeDefend(stack);
	else
		return BattleAction::makeHeal(stack, woundHpToStack.rbegin()->second); //last element of the woundHpToStack is the most wounded stack
}

void CBattleAI::yourTacticPhase(const BattleID & battleID, int distance)
{
	cb->battleMakeTacticAction(battleID, BattleAction::makeEndOFTacticPhase(cb->getBattle(battleID)->battleGetTacticsSide()));
}

static float getStrengthRatio(std::shared_ptr<CBattleInfoCallback> cb, BattleSide side)
{
	auto stacks = cb->battleGetAllStacks();
	auto our = 0;
	auto enemy = 0;

	for(auto stack : stacks)
	{
		auto creature = stack->creatureId().toCreature();

		if(!creature)
			continue;

		if(stack->unitSide() == side)
			our += stack->getCount() * creature->getAIValue();
		else
			enemy += stack->getCount() * creature->getAIValue();
	}

	return enemy == 0 ? 1.0f : static_cast<float>(our) / enemy;
}

int getSimulationTurnsCount(const StartInfo * startInfo)
{
	return startInfo->difficulty < 4 ? 2 : 10;
}

void CBattleAI::activeStack(const BattleID & battleID, const CStack * stack )
{
	LOG_TRACE_PARAMS(logAi, "stack: %s", stack->nodeName());

	auto timeElapsed = [](std::chrono::time_point<std::chrono::high_resolution_clock> start) -> uint64_t
	{
		auto end = std::chrono::high_resolution_clock::now();

		return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	};

	BattleAction result = BattleAction::makeDefend(stack);

	auto start = std::chrono::high_resolution_clock::now();

	if(stack->creatureId() == CreatureID::CATAPULT)
	{
		cb->battleMakeUnitAction(battleID, useCatapult(battleID, stack));
		return;
	}
	if(stack->hasBonusOfType(BonusType::SIEGE_WEAPON) && stack->hasBonusOfType(BonusType::HEALER))
	{
		cb->battleMakeUnitAction(battleID, useHealingTent(battleID, stack));
		return;
	}

#if BATTLE_TRACE_LEVEL>=1
	logAi->trace("Build evaluator and targets");
#endif

	BattleEvaluator evaluator(
		env, cb, stack, playerID, battleID, side,
		getStrengthRatio(cb->getBattle(battleID), side),
		getSimulationTurnsCount(env->game()->getStartInfo()));

	result = evaluator.selectStackAction(stack);

	if(autobattlePreferences.enableSpellsUsage && evaluator.canCastSpell())
	{
		auto spelCasted = evaluator.attemptCastingSpell(stack);

		if(spelCasted)
			return;
	}

	logAi->trace("Spellcast attempt completed in %lld", timeElapsed(start));

	if(auto action = considerFleeingOrSurrendering(battleID))
	{
		cb->battleMakeUnitAction(battleID, *action);
		return;
	}

	if(result.actionType == EActionType::DEFEND)
	{
		movesSkippedByDefense++;
	}
	else if(result.actionType != EActionType::WAIT)
	{
		movesSkippedByDefense = 0;
	}

	logAi->trace("BattleAI decision made in %lld", timeElapsed(start));

	cb->battleMakeUnitAction(battleID, result);
}

BattleAction CBattleAI::useCatapult(const BattleID & battleID, const CStack * stack)
{
	BattleAction attack;
	BattleHex targetHex = BattleHex::INVALID;

	if(cb->getBattle(battleID)->battleGetGateState() == EGateState::CLOSED)
	{
		targetHex = cb->getBattle(battleID)->wallPartToBattleHex(EWallPart::GATE);
	}
	else
	{
		std::array wallParts {
			EWallPart::KEEP,
			EWallPart::BOTTOM_TOWER,
			EWallPart::UPPER_TOWER,
			EWallPart::BELOW_GATE,
			EWallPart::OVER_GATE,
			EWallPart::BOTTOM_WALL,
			EWallPart::UPPER_WALL
		};

		for(auto wallPart : wallParts)
		{
			auto wallState = cb->getBattle(battleID)->battleGetWallState(wallPart);

			if(wallState != EWallState::NONE && wallState != EWallState::DESTROYED)
			{
				targetHex = cb->getBattle(battleID)->wallPartToBattleHex(wallPart);
				break;
			}
		}
	}

	if(!targetHex.isValid())
	{
		return BattleAction::makeDefend(stack);
	}

	attack.aimToHex(targetHex);
	attack.actionType = EActionType::CATAPULT;
	attack.side = side;
	attack.stackNumber = stack->unitId();

	movesSkippedByDefense = 0;

	return attack;
}

void CBattleAI::battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide Side, bool replayAllowed)
{
	LOG_TRACE(logAi);
	side = Side;
}

void CBattleAI::print(const std::string &text) const
{
	logAi->trace("%s Battle AI[%p]: %s", playerID.toString(), this, text);
}

std::optional<BattleAction> CBattleAI::considerFleeingOrSurrendering(const BattleID & battleID)
{
	BattleStateInfoForRetreat bs;

	bs.canFlee = cb->getBattle(battleID)->battleCanFlee();
	bs.canSurrender = cb->getBattle(battleID)->battleCanSurrender(playerID);
	bs.ourSide = cb->getBattle(battleID)->battleGetMySide();
	bs.ourHero = cb->getBattle(battleID)->battleGetMyHero();
	bs.enemyHero = nullptr;

	for(auto stack : cb->getBattle(battleID)->battleGetAllStacks(false))
	{
		if(stack->alive())
		{
			if(stack->unitSide() == bs.ourSide)
				bs.ourStacks.push_back(stack);
			else
			{
				bs.enemyStacks.push_back(stack);
				bs.enemyHero = cb->getBattle(battleID)->battleGetOwnerHero(stack);
			}
		}
	}

	bs.turnsSkippedByDefense = movesSkippedByDefense / bs.ourStacks.size();

	if(!bs.canFlee && !bs.canSurrender)
	{
		return std::nullopt;
	}

	auto result = cb->makeSurrenderRetreatDecision(battleID, bs);

	if(!result && bs.canFlee && bs.turnsSkippedByDefense > 30)
	{
		return BattleAction::makeRetreat(bs.ourSide);
	}

	return result;
}



