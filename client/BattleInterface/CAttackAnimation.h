#pragma once

#include "CBattleStackAnimation.h"
#include "../CAnimation.h"
#include "../../lib/SBattleHex.h"

class CBattleInterface;
class CStack;

/*
 * CAttackAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// This class is responsible for managing the battle attack animation
class CAttackAnimation : public CBattleStackAnimation
{
protected:
	SBattleHex dest; //attacked hex
	bool shooting;
	CCreatureAnim::EAnimType group; //if shooting is true, print this animation group
	const CStack *attackedStack;
	const CStack *attackingStack;
	int attackingStackPosBeforeReturn; //for stacks with return_after_strike feature
public:
	void nextFrame();
	bool checkInitialConditions();

	CAttackAnimation(CBattleInterface *_owner, const CStack *attacker, SBattleHex _dest, const CStack *defender);
};