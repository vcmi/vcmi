/*
 * OverlayTrigger.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

/// Visibility of the name overlay, which shows the names of map objects and town buildings.
/// Its trigger is the Alt key or a two-finger touch, used either as hold or, with the
/// 'overlayToggleMode' setting, as toggle.
class OverlayTrigger
{
	bool active = false;
	bool triggerWasDown = false;

public:
	/// Reads the trigger and returns whether the overlay should be visible right now.
	/// May be called any number of times per frame: the trigger can only change between
	/// frames, when input events are fetched, so every call of a frame gives the same answer.
	bool isActive();
};
