/*
 * OverlayTrigger.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "OverlayTrigger.h"

#include "../GameEngine.h"
#include "events/InputHandler.h"

#include "../../lib/CConfigHandler.h"

bool OverlayTrigger::isActive()
{
	if(!settings["general"]["enableOverlay"].Bool())
	{
		active = false;
		triggerWasDown = false;
		return false;
	}

	bool triggerDown = ENGINE->isKeyboardAltDown() || ENGINE->input().getNumTouchFingers() == 2;

	if(settings["general"]["overlayToggleMode"].Bool())
	{
		// a tap of the trigger flips the overlay, which then stays as it is
		if(triggerDown && !triggerWasDown)
			active = !active;
	}
	else
		active = triggerDown;

	triggerWasDown = triggerDown;
	return active;
}
