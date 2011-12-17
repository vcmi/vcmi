#pragma once

#include "../UIFramework/CIntObject.h"

class CBattleInterface;
class CPicture;
class AdventureMapButton;
class CHighlightableButton;
class CHighlightableButtonsGroup;
class CLabel;
struct SDL_Rect;

/*
 * CBattleOptionsWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Class which manages the battle options window
class CBattleOptionsWindow : public CIntObject
{
private:
	CBattleInterface *myInt;
	CPicture *background;
	AdventureMapButton *setToDefault, *exit;
	CHighlightableButton *viewGrid, *movementShadow, *mouseShadow;
	CHighlightableButtonsGroup *animSpeeds;

	std::vector<CLabel*> labels;
public:
	CBattleOptionsWindow(const SDL_Rect &position, CBattleInterface *owner); //c-tor

	void bDefaultf(); //default button callback
	void bExitf(); //exit button callback
};