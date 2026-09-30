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

#include "HealAction.h"
#include "NoTargetActions.h"
#include "ShootAction.h"
#include "WalkAction.h"

#include "../BattleAction.h"
#include "../CBattleInfoCallback.h"
#include "../CUnitState.h"
#include "../../spells/Problem.h"

static constexpr SkipTurnAction skipTurnAction;
static constexpr BadMoraleAction badMoraleAction;
static constexpr WaitAction waitAction;
static constexpr DefendAction defendAction;
static constexpr EndTacticsAction endTacticsAction;
static constexpr RetreatAction retreatAction;
static constexpr SurrenderAction surrenderAction;
static constexpr WalkAction walkAction;
static constexpr ShootAction shootAction;
static constexpr HealAction healAction;

namespace
{
struct RegisteredActionType
{
	EActionType actionType;
	const BattleActionType * type;
};
}

static constexpr std::array<RegisteredActionType, 10> registeredTypes = {{
	{EActionType::NO_ACTION, &skipTurnAction},
	{EActionType::BAD_MORALE, &badMoraleAction},
	{EActionType::WAIT, &waitAction},
	{EActionType::DEFEND, &defendAction},
	{EActionType::END_TACTIC_PHASE, &endTacticsAction},
	{EActionType::RETREAT, &retreatAction},
	{EActionType::SURRENDER, &surrenderAction},
	{EActionType::WALK, &walkAction},
	{EActionType::SHOOT, &shootAction},
	{EActionType::STACK_HEAL, &healAction},
}};

const BattleActionType * BattleActionType::find(const BattleAction & action)
{
	return find(action.actionType);
}

const BattleActionType * BattleActionType::find(EActionType actionType)
{
	for(const auto & registered : registeredTypes)
		if(registered.actionType == actionType)
			return registered.type;

	return nullptr;
}

void BattleActionType::collectAllOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out)
{
	for(const auto & registered : registeredTypes)
		registered.type->collectOptions(battle, actor, out);
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

void BattleActionType::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
}

int BattleActionType::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	throw std::runtime_error("Priority requested from a battle action type that offers no options");
}

bool BattleActionType::isLegal(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	spells::detail::ProblemImpl problem;
	return validate(game, battle, build(battle, option, context), problem);
}

ActionPreview BattleActionType::preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	throw std::runtime_error("Preview requested from a battle action type that offers no options");
}

BattleAction BattleActionType::build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	throw std::runtime_error("Action built from a battle action type that offers no options");
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
