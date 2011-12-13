#pragma once

#include "CBattleStackAnimation.h"


class CBattleInterface;
class CStack;

/*
 * CMovementEndAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Move end animation of a creature
class CMovementEndAnimation : public CBattleStackAnimation
{
private:
	SHexField destinationTile;
public:
	bool init();
	void nextFrame();
	void endAnim();

	CMovementEndAnimation(CBattleInterface *_owner, const CStack *_stack, SHexField destTile);
};