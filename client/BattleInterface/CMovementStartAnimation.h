#pragma once

#include "CBattleStackAnimation.h"

class CBattleInterface;
class CStack;

/*
 * CMovementStartAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Move start animation of a creature
class CMovementStartAnimation : public CBattleStackAnimation
{
public:
	bool init();
	void nextFrame();
	void endAnim();

	CMovementStartAnimation(CBattleInterface *_owner, const CStack *_stack);
};