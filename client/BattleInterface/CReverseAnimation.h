#pragma once

#include "CBattleStackAnimation.h"


class CStack;

/*
 * CReverseAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Class responsible for animation of stack chaning direction (left <-> right)
class CReverseAnimation : public CBattleStackAnimation
{
private:
	int partOfAnim; //1 - first, 2 - second
	bool secondPartSetup;
	SHexField hex;
public:
	bool priority; //true - high, false - low
	bool init();
	void nextFrame();

	void setupSecondPart();
	void endAnim();

	CReverseAnimation(CBattleInterface *_owner, const CStack *stack, SHexField dest, bool _priority);
};