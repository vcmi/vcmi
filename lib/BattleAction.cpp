#define VCMI_DLL
#include "BattleAction.h"
#include "CGameState.h"

BattleAction::BattleAction()
{
	side = -1;
	stackNumber = -1;
	actionType = INVALID;
	destinationTile = -1;
	additionalInfo = -1;
}

BattleAction BattleAction::makeDefend(const CStack *stack)
{
	BattleAction ba;
	ba.side = !stack->attackerOwned;
	ba.actionType = DEFEND;
	ba.stackNumber = stack->ID;
	return ba;
}

BattleAction BattleAction::makeMeleeAttack(const CStack *stack) /*WARNING: stacks must be neighbouring! */
{
	BattleAction ba;
	ba.side = !stack->attackerOwned;
	ba.actionType = WAIT;
	ba.stackNumber = stack->ID;
	return ba;
}

BattleAction BattleAction::makeWait(const CStack *stack)
{
	BattleAction ba;
	ba.side = !stack->attackerOwned;
	ba.actionType = WAIT;
	ba.stackNumber = stack->ID;
	return ba;
}

BattleAction BattleAction::makeShotAttack(const CStack *shooter, const CStack *target)
{
	BattleAction ba;
	ba.side = !shooter->attackerOwned;
	ba.actionType = SHOOT;
	ba.stackNumber = shooter->ID;
	ba.destinationTile = target->position;
	return ba;
}

BattleAction BattleAction::makeMove(const CStack *stack, THex dest)
{
	BattleAction ba;
	ba.side = !stack->attackerOwned;
	ba.actionType = WALK;
	ba.stackNumber = stack->ID;
	ba.destinationTile = dest;
	return ba;
}