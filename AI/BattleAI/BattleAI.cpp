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
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/BattleStateInfoForRetreat.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/CStack.h" // TODO: remove
                              // Eventually only IBattleInfoCallback and battle::Unit should be used,
                              // CUnitState should be private and CStack should be removed completely

#define LOGL(text) print(text)
#define LOGFL(text, formattingEl) print(boost::str(boost::format(text) % formattingEl))

CBattleAI::CBattleAI()
	: side(-1),
	wasWaitingForRealize(false),
	wasUnlockingGs(false)
{
}

CBattleAI::~CBattleAI()
{
	if(cb)
	{
		//Restore previous state of CB - it may be shared with the main AI (like VCAI)
		cb->waitTillRealize = wasWaitingForRealize;
		cb->unlockGsWhenWaiting = wasUnlockingGs;
	}
}

void CBattleAI::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB)
{
	setCbc(CB);
	env = ENV;
	cb = CB;
	playerID = *CB->getPlayerID(); //TODO should be sth in callback
	wasWaitingForRealize = CB->waitTillRealize;
	wasUnlockingGs = CB->unlockGsWhenWaiting;
	CB->waitTillRealize = false;
	CB->unlockGsWhenWaiting = false;
	movesSkippedByDefense = 0;
}

BattleAction CBattleAI::useHealingTent(const CStack *stack)
{
	auto healingTargets = cb->battleGetStacks(CBattleInfoEssentials::ONLY_MINE);
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

void CBattleAI::yourTacticPhase(int distance)
{
	cb->battleMakeTacticAction(BattleAction::makeEndOFTacticPhase(cb->battleGetTacticsSide()));
}

float getStrengthRatio(std::shared_ptr<CBattleCallback> cb, int side)
{
	auto stacks = cb->battleGetAllStacks();
	auto our = 0, enemy = 0;

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

void CBattleAI::activeStack(const CStack * stack )
{
	LOG_TRACE_PARAMS(logAi, "stack: %s", stack->nodeName());

	auto timeElapsed = [](std::chrono::time_point<std::chrono::high_resolution_clock> start) -> uint64_t
	{
		auto end = std::chrono::high_resolution_clock::now();

		return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	};

	BattleAction result = BattleAction::makeDefend(stack);
	setCbc(cb); //TODO: make solid sure that AIs always use their callbacks (need to take care of event handlers too)

	auto start = std::chrono::high_resolution_clock::now();

	try
	{
		if(stack->creatureId() == CreatureID::CATAPULT)
		{
			cb->battleMakeUnitAction(useCatapult(stack));
			return;
		}
		if(stack->hasBonusOfType(BonusType::SIEGE_WEAPON) && stack->hasBonusOfType(BonusType::HEALER))
		{
			cb->battleMakeUnitAction(useHealingTent(stack));
			return;
		}

#if BATTLE_TRACE_LEVEL>=1
		logAi->trace("Build evaluator and targets");
#endif

		BattleEvaluator evaluator(env, cb, stack, playerID, side, getStrengthRatio(cb, side));

		result = evaluator.selectStackAction(stack);

		if(!skipCastUntilNextBattle && evaluator.canCastSpell())
		{
			auto spelCasted = evaluator.attemptCastingSpell(stack);

			if(spelCasted)
				return;
			
			skipCastUntilNextBattle = true;
		}

		logAi->trace("Spellcast attempt completed in %lld", timeElapsed(start));

		if(auto action = considerFleeingOrSurrendering())
		{
			cb->battleMakeUnitAction(*action);
			return;
		}
	}
	catch(boost::thread_interrupted &)
	{
		throw;
	}
	catch(std::exception &e)
	{
		logAi->error("Exception occurred in %s %s",__FUNCTION__, e.what());
	}

	if(result.actionType == EActionType::DEFEND)
	{
		movesSkippedByDefense++;
	}
	else if(result.actionType != EActionType::WAIT)
	{
		movesSkippedByDefense = 0;
	}

	logAi->trace("BattleAI decission made in %lld", timeElapsed(start));

	cb->battleMakeUnitAction(result);
}

BattleAction CBattleAI::useCatapult(const CStack * stack)
{
	BattleAction attack;
	BattleHex targetHex = BattleHex::INVALID;

	if(cb->battleGetGateState() == EGateState::CLOSED)
	{
		targetHex = cb->wallPartToBattleHex(EWallPart::GATE);
	}
	else
	{
		EWallPart wallParts[] = {
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
			auto wallState = cb->battleGetWallState(wallPart);

			if(wallState == EWallState::REINFORCED || wallState == EWallState::INTACT || wallState == EWallState::DAMAGED)
			{
				targetHex = cb->wallPartToBattleHex(wallPart);
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

void CBattleAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool Side, bool replayAllowed)
{
	LOG_TRACE(logAi);
	side = Side;

	skipCastUntilNextBattle = false;
}

void CBattleAI::print(const std::string &text) const
{
	logAi->trace("%s Battle AI[%p]: %s", playerID.getStr(), this, text);
}

std::optional<BattleAction> CBattleAI::considerFleeingOrSurrendering()
{
	BattleStateInfoForRetreat bs;

	bs.canFlee = cb->battleCanFlee();
	bs.canSurrender = cb->battleCanSurrender(playerID);
	bs.ourSide = cb->battleGetMySide();
	bs.ourHero = cb->battleGetMyHero(); 
	bs.enemyHero = nullptr;

	for(auto stack : cb->battleGetAllStacks(false))
	{
		if(stack->alive())
		{
			if(stack->unitSide() == bs.ourSide)
				bs.ourStacks.push_back(stack);
			else
			{
				bs.enemyStacks.push_back(stack);
				bs.enemyHero = cb->battleGetOwnerHero(stack);
			}
		}
	}

	bs.turnsSkippedByDefense = movesSkippedByDefense / bs.ourStacks.size();

	if(!bs.canFlee && !bs.canSurrender)
	{
		return std::nullopt;
	}

	auto result = cb->makeSurrenderRetreatDecision(bs);

	if(!result && bs.canFlee && bs.turnsSkippedByDefense > 30)
	{
		return BattleAction::makeRetreat(bs.ourSide);
	}

	return result;
}



