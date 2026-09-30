/*
 * BattleActionType.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleActionType.h"

#include "NoTargetActions.h"

#include "../BattleAction.h"
#include "../CBattleInfoCallback.h"
#include "../CUnitState.h"
#include "../../spells/Problem.h"

const BattleActionType * BattleActionType::find(const BattleAction & action)
{
	static constexpr SkipTurnAction skipTurn;
	static constexpr BadMoraleAction badMorale;
	static constexpr WaitAction wait;
	static constexpr DefendAction defend;
	static constexpr EndTacticsAction endTactics;
	static constexpr RetreatAction retreat;
	static constexpr SurrenderAction surrender;

	switch(action.actionType)
	{
		case EActionType::NO_ACTION:
			return &skipTurn;
		case EActionType::BAD_MORALE:
			return &badMorale;
		case EActionType::WAIT:
			return &wait;
		case EActionType::DEFEND:
			return &defend;
		case EActionType::END_TACTIC_PHASE:
			return &endTactics;
		case EActionType::RETREAT:
			return &retreat;
		case EActionType::SURRENDER:
			return &surrender;
		default:
			return nullptr;
	}
}

bool BattleActionType::isUnitAction() const
{
	return false;
}

bool BattleActionType::isTacticsAction() const
{
	return false;
}

bool BattleActionType::isBattleEndAction() const
{
	return false;
}

void BattleActionType::applyStartState(battle::CUnitState & actor, const BattleAction & action) const
{
	actor.waiting = false;
	actor.movedThisRound = true;
}

MetaString BattleActionType::getStartLogLine(const CBattleInfoCallback & battle, const BattleAction & action) const
{
	return {};
}

bool BattleActionType::checkUnitCanAct(const CBattleInfoCallback & battle, const battle::Unit * unit, spells::Problem & problem)
{
	if(!unit)
	{
		problem.add(MetaString::createFromRawString("No such stack!"));
		return false;
	}

	if(!unit->alive())
	{
		problem.add(MetaString::createFromRawString("This stack is dead: " + unit->getDescription()));
		return false;
	}

	if(battle.battleTacticDist())
	{
		if(unit->unitSide() != battle.battleGetTacticsSide())
		{
			problem.add(MetaString::createFromRawString("This is not a stack of side that has tactics!"));
			return false;
		}
	}
	else if(unit != battle.battleActiveUnit())
	{
		problem.add(MetaString::createFromRawString("Action has to be about active stack!"));
		return false;
	}

	return true;
}

const battle::Unit & BattleActionType::getActor(const CBattleInfoCallback & battle, const BattleAction & action)
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);
	if(!unit)
		throw std::runtime_error("Battle action without an existing actor: " + action.toString());
	return *unit;
}
