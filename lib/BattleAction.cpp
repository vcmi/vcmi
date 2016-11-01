#include "StdInc.h"
#include "BattleAction.h"

#include "BattleState.h"

/*
 * BattleAction.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

using namespace Battle;

BattleAction::BattleAction():
	side(-1),
	stackNumber(-1),
	actionType(INVALID),
	destinationTile(-1),
	additionalInfo(-1),
	selectedStack(-1)
{
}

BattleAction BattleAction::makeHeal(const CStack *healer, const CStack *healed)
{
	BattleAction ba;
	ba.side = !healer->attackerOwned;
	ba.actionType = STACK_HEAL;
	ba.stackNumber = healer->ID;
	ba.destinationTile = healed->position;
	return ba;
}

BattleAction BattleAction::makeDefend(const CStack *stack)
{
	BattleAction ba;
	ba.side = !stack->attackerOwned;
	ba.actionType = DEFEND;
	ba.stackNumber = stack->ID;
	return ba;
}


BattleAction BattleAction::makeMeleeAttack(const CStack *stack, const CStack * attacked, BattleHex attackFrom /*= BattleHex::INVALID*/)
{
	BattleAction ba;
	ba.side = !stack->attackerOwned;
	ba.actionType = WALK_AND_ATTACK;
	ba.stackNumber = stack->ID;
	ba.destinationTile = attackFrom;
	ba.additionalInfo = attacked->position;
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

BattleAction BattleAction::makeMove(const CStack *stack, BattleHex dest)
{
	BattleAction ba;
	ba.side = !stack->attackerOwned;
	ba.actionType = WALK;
	ba.stackNumber = stack->ID;
	ba.destinationTile = dest;
	return ba;
}

BattleAction BattleAction::makeEndOFTacticPhase(ui8 side)
{
	BattleAction ba;
	ba.side = side;
	ba.actionType = END_TACTIC_PHASE;
	return ba;
}

std::ostream & operator<<(std::ostream & os, const BattleAction & ba)
{
	std::stringstream actionTypeStream;
	actionTypeStream << ba.actionType;

	return os << boost::str(boost::format("{BattleAction: side '%d', stackNumber '%d', actionType '%s', destinationTile '%s', additionalInfo '%d', selectedStack '%d'}")
			% static_cast<int>(ba.side) % ba.stackNumber % actionTypeStream.str() % ba.destinationTile % ba.additionalInfo % ba.selectedStack);
}
