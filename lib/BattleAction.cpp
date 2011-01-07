#define VCMI_DLL
#include "BattleAction.h"
#include "BattleState.h"

/*
 * BattleAction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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

BattleAction BattleAction::makeMeleeAttack( const CStack *stack, const CStack * attacked, std::vector<THex> reachableByAttacker )
{
	BattleAction ba;
	ba.side = !stack->attackerOwned;
	ba.actionType = WALK_AND_ATTACK;
	ba.stackNumber = stack->ID;
	ba.destinationTile = -1;
	for (int g=0; g<reachableByAttacker.size(); ++g)
	{
		if (THex::mutualPosition(reachableByAttacker[g], attacked->position) >= 0 )
		{
			ba.destinationTile = reachableByAttacker[g];
			break;
		}
	}

	if (ba.destinationTile == -1)
	{
		//we couldn't determine appropriate pos
		//TODO: should we throw an exception?
		return makeDefend(stack);
	}

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

BattleAction BattleAction::makeMove(const CStack *stack, THex dest)
{
	BattleAction ba;
	ba.side = !stack->attackerOwned;
	ba.actionType = WALK;
	ba.stackNumber = stack->ID;
	ba.destinationTile = dest;
	return ba;
}