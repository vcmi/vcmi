#pragma once


#include "BattleHex.h"

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

struct DLL_LINKAGE BattleAction
{
	ui8 side; //who made this action: false - left, true - right player
	ui32 stackNumber;//stack ID, -1 left hero, -2 right hero,
	Battle::ActionType actionType; //use ActionType enum for values
	BattleHex destinationTile;
	si32 additionalInfo; // e.g. spell number if type is 1 || 10; tile to attack if type is 6
	si32 selectedStack; //spell subject for teleport / sacrifice

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & side & stackNumber & actionType & destinationTile & additionalInfo & selectedStack;
	}

	BattleAction();

	static BattleAction makeHeal(const CStack *healer, const CStack *healed);
	static BattleAction makeDefend(const CStack *stack);
	static BattleAction makeWait(const CStack *stack);
	static BattleAction makeMeleeAttack(const CStack *stack, const CStack * attacked, BattleHex attackFrom = BattleHex::INVALID);
	static BattleAction makeShotAttack(const CStack *shooter, const CStack *target);
	static BattleAction makeMove(const CStack *stack, BattleHex dest);
	static BattleAction makeEndOFTacticPhase(ui8 side);
};

DLL_EXPORT std::ostream & operator<<(std::ostream & os, const BattleAction & ba);
