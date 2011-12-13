#pragma once

class CBattleInterface;

/*
 * CBattleAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Base class of battle animations
class CBattleAnimation
{
protected:
	CBattleInterface * owner;
public:
	virtual bool init()=0; //to be called - if returned false, call again until returns true
	virtual void nextFrame()=0; //call every new frame
	virtual void endAnim(); //to be called mostly internally; in this class it removes animation from pendingAnims list

	bool isEarliest(bool perStackConcurrency); //determines if this animation is earliest of all

	ui32 ID; //unique identifier

	CBattleAnimation(CBattleInterface * _owner);
};