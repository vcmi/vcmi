#pragma once

#include "../GUIBase.h"

class CBattleInterface;
class CStack;
struct SDL_MouseMotionEvent;

/*
 * CHexFieldControl.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Class which stands for a single hex field on a battlefield
class CHexFieldControl : public CIntObject
{
private:
	bool setAlterText; //if true, this hex has set alternative text in console and will clean it
public:
	ui32 myNumber; //number of hex in commonly used format
	bool accessible; //if true, this hex is accessible for units
	//CStack * ourStack;
	bool hovered, strictHovered; //for determining if hex is hovered by mouse (this is different problem than hex's graphic hovering)
	CBattleInterface * myInterface; //interface that owns me
	static Point getXYUnitAnim(const int &hexNum, const bool &attacker, const CStack *creature, const CBattleInterface *cbi); //returns (x, y) of left top corner of animation
	
	//for user interactions
	void hover (bool on);
	void activate();
	void deactivate();
	void mouseMoved (const SDL_MouseMotionEvent &sEvent);
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	CHexFieldControl();
};