/*
 * ClassicBattleController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"
#include "ClassicBattleController.h"

#include "../../../lib/CStack.h"
#include "../../../lib/StartInfo.h"
#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/battle/CPlayerBattleCallback.h"
#include "../../../lib/callback/CBattleCallback.h"
#include "../../../lib/callback/IGameInfoCallback.h"
#include "ClassicBattleDecision.h"
#include "ClassicBattleRng.h"
#include "ClassicBattleStateView.h"
#include <vcmi/Environment.h>

ClassicBattleController::ClassicBattleController(
	std::shared_ptr<Environment> env,
	std::shared_ptr<CBattleCallback> cb,
	PlayerColor playerID,
	std::shared_ptr<IClassicBattleAIRng> randomGenerator,
	std::shared_ptr<ClassicDecisionTrace> trace
)
	: env(std::move(env)),
	cb(std::move(cb)),
	playerID(playerID),
	side(BattleSide::NONE),
	randomGenerator(std::move(randomGenerator)),
	trace(std::move(trace))
{
	if(!this->randomGenerator)
		this->randomGenerator = std::make_shared<ClassicBattleAIRng>();
}

void ClassicBattleController::battleStart(const BattleID & battleID, BattleSide side)
{
	this->side = side;
	currentBattleID = battleID;
	tacticsStackIDs.clear();
	tacticsCursor = 0;
	movingTacticsStack = std::numeric_limits<uint32_t>::max();
}

void ClassicBattleController::battleEnd(const BattleID & battleID)
{
	side = BattleSide::NONE;
	currentBattleID = BattleID::NONE;
	tacticsStackIDs.clear();
	tacticsCursor = 0;
	movingTacticsStack = std::numeric_limits<uint32_t>::max();
}

void ClassicBattleController::activeStack(
	const BattleID & battleID,
	const CStack * stack,
	const AutocombatPreferences & preferences)
{
	const BattleAction action = decideStackAction(battleID, stack, preferences);
	if(action.actionType == EActionType::HERO_SPELL)
		cb->battleMakeSpellAction(battleID, action);
	else
		cb->battleMakeUnitAction(battleID, action);
}

BattleAction ClassicBattleController::decideStackAction(
	const BattleID & battleID,
	const CStack * stack,
	const AutocombatPreferences & preferences)
{
	const auto battle = cb->getBattle(battleID);
	const int32_t difficulty = env->game()->getStartInfo()->difficulty;
	return ClassicBattleDecision::decide(
		env.get(), battle, side, stack, difficulty, preferences,
		randomGenerator, trace, ClassicDecisionEntryPoint::FULL_PIPELINE);
}

void ClassicBattleController::yourTacticPhase(
	const BattleID & battleID,
	int distance,
	const AutocombatPreferences & preferences)
{
	currentBattleID = battleID;
	tacticsStackIDs.clear();
	tacticsCursor = 0;
	movingTacticsStack = std::numeric_limits<uint32_t>::max();
	if(!preferences.enableTacticsUsage)
	{
		cb->battleMakeTacticAction(battleID, BattleAction::makeEndOFTacticPhase(side));
		return;
	}

	ClassicBattleStateView view(cb->getBattle(battleID));
	for(const CStack * stack : view.orderedStacks())
	{
		if(stack->unitSide() == side && stack->canMove())
			tacticsStackIDs.push_back(stack->unitId());
	}
	advanceTactics();
}

void ClassicBattleController::actionFinished(const BattleID & battleID, const BattleAction & action)
{
	if(battleID != currentBattleID || movingTacticsStack == std::numeric_limits<uint32_t>::max())
		return;
	if(action.actionType != EActionType::WALK || action.stackNumber != movingTacticsStack)
		return;
	movingTacticsStack = std::numeric_limits<uint32_t>::max();
	if(cb->getBattle(battleID)->battleTacticDist() > 0)
		advanceTactics();
}

void ClassicBattleController::advanceTactics()
{
	const auto battle = cb->getBattle(currentBattleID);
	while(tacticsCursor < tacticsStackIDs.size())
	{
		const uint32_t stackID = tacticsStackIDs[tacticsCursor++];
		const auto * stack = battle->battleGetStackByID(stackID, true);
		if(!stack)
			continue;
		const auto action = ClassicBattleDecision::decide(
			env.get(), battle, side, stack, env->game()->getStartInfo()->difficulty,
			AutocombatPreferences{}, randomGenerator, trace,
			ClassicDecisionEntryPoint::DO_COMP_AI);
		// WAIT skips this stack during deployment; only movement is submitted
		// to the server. The classic END_TACTIC_PHASE decision ends deployment.
		if(action.actionType == EActionType::WAIT)
			continue;
		if(action.actionType == EActionType::END_TACTIC_PHASE)
			break;
		if(action.actionType != EActionType::WALK)
			continue;
		movingTacticsStack = stackID;
		cb->battleMakeTacticAction(currentBattleID, action);
		return;
	}
	cb->battleMakeTacticAction(currentBattleID, BattleAction::makeEndOFTacticPhase(side));
}
