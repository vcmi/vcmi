#pragma once
#ifndef __BATTLEACTION_H__
#define __BATTLEACTION_H__

#include "../global.h"
/*
 * BattleAction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// A struct which handles battle actions like defending, walking,... - represents a creature stack in a battle
class CStack;

struct DLL_EXPORT BattleAction
{
	ui8 side; //who made this action: false - left, true - right player
	ui32 stackNumber;//stack ID, -1 left hero, -2 right hero,
	enum ActionType
	{
		END_TACTIC_PHASE = -2, INVALID = -1, NO_ACTION = 0, HERO_SPELL, WALK, DEFEND, RETREAT, SURRENDER, WALK_AND_ATTACK, SHOOT, WAIT, CATAPULT, MONSTER_SPELL, BAD_MORALE,
		STACK_HEAL, DAEMON_SUMMONING
	};
	si8 actionType; //use ActionType enum for values
	THex destinationTile;
	si32 additionalInfo; // e.g. spell number if type is 1 || 10; tile to attack if type is 6
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & side & stackNumber & actionType & destinationTile & additionalInfo;
	}

	BattleAction();

	static BattleAction makeDefend(const CStack *stack);
	static BattleAction makeWait(const CStack *stack);
	static BattleAction makeMeleeAttack(const CStack *stack, const CStack * attacked, THex attackFrom = THex::INVALID);
	static BattleAction makeShotAttack(const CStack *shooter, const CStack *target);
	static BattleAction makeMove(const CStack *stack, THex dest);
	static BattleAction makeEndOFTacticPhase(ui8 side);
};
#endif // __BATTLEACTION_H__
