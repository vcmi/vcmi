#pragma once

#include "CAttackAnimation.h"


class CBattleInterface;
class CStack;

/*
 * CMeleeAttackAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Hand-to-hand attack
class CMeleeAttackAnimation : public CAttackAnimation
{
public:
	bool init();
	void nextFrame();
	void endAnim();

	CMeleeAttackAnimation(CBattleInterface *_owner, const CStack *attacker, SBattleHex _dest, const CStack *_attacked);
};