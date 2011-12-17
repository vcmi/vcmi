#pragma once

#include "CBattleAnimation.h"
#include "../../lib/SBattleHex.h"

class CStack;
class CBattleInterface;
class CCreatureAnimation;

/*
 * CBattleStackAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Sub-class which is responsible for managing the battle stack animation.
class CBattleStackAnimation : public CBattleAnimation
{
public:
	const CStack * stack; //id of stack whose animation it is

	CBattleStackAnimation(CBattleInterface * _owner, const CStack * _stack);
	static bool isToReverseHlp(SBattleHex hexFrom, SBattleHex hexTo, bool curDir); //helper for isToReverse
	static bool isToReverse(SBattleHex hexFrom, SBattleHex hexTo, bool curDir /*if true, creature is in attacker's direction*/, bool toDoubleWide, bool toDir); //determines if creature should be reversed (it stands on hexFrom and should 'see' hexTo)

	CCreatureAnimation *myAnim(); //animation for our stack
};