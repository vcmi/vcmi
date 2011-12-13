#pragma once

#include "CBattleAnimation.h"

class CBattleInterface;

/*
 * CDummyAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CDummyAnimation : public CBattleAnimation
{
private:
	int counter;
	int howMany;
public:
	bool init();
	void nextFrame();
	void endAnim();

	CDummyAnimation(CBattleInterface *_owner, int howManyFrames);
};