#pragma once;
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

class CStack;

struct DLL_EXPORT BattleAction
{
	ui8 side; //who made this action: false - left, true - right player
	ui32 stackNumber;//stack ID, -1 left hero, -2 right hero,
	enum ActionType
	{
		INVALID = -1, NO_ACTION = 0, HERO_SPELL, WALK, DEFEND, RETREAT, SURRENDER, WALK_AND_ATTACK, SHOOT, WAIT, CATAPULT, MONSTER_SPELL, BAD_MORALE, STACK_HEAL
	};
	ui8 actionType; //    0 = No action;   1 = Hero cast a spell   2 = Walk   3 = Defend   4 = Retreat from the battle
		//5 = Surrender   6 = Walk and Attack   7 = Shoot    8 = Wait   9 = Catapult
		//10 = Monster casts a spell (i.e. Faerie Dragons)	11 - Bad morale freeze	12 - stacks heals another stack
	ui16 destinationTile;
	si32 additionalInfo; // e.g. spell number if type is 1 || 10; tile to attack if type is 6
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & side & stackNumber & actionType & destinationTile & additionalInfo;
	}

	BattleAction();

	static BattleAction makeDefend(const CStack *stack);
	static BattleAction makeWait(const CStack *stack);
	static BattleAction makeMeleeAttack(const CStack *stack); //WARNING: stacks must be neighbouring!;
	static BattleAction makeShotAttack(const CStack *shooter, const CStack *target);
	static BattleAction makeMove(const CStack *stack, THex dest);
};
#endif // __BATTLEACTION_H__
