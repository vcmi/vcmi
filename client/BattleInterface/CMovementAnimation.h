#pragma once

#include "CBattleStackAnimation.h"


class CBattleInterface;
class CStack;

/*
 * CMovementAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Move animation of a creature
class CMovementAnimation : public CBattleStackAnimation
{
private:
	std::vector<SHexField> destTiles; //destination
	SHexField nextHex;
	ui32 nextPos;
	int distance;
	double stepX, stepY; //how far stack is moved in one frame
	double posX, posY;
	int steps, whichStep;
	int curStackPos; //position of stack before move
public:
	bool init();
	void nextFrame();
	void endAnim();

	CMovementAnimation(CBattleInterface *_owner, const CStack *_stack, std::vector<SHexField> _destTiles, int _distance);
};