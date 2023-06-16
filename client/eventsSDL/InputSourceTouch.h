/*
* InputSourceTouch.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "../../lib/Point.h"

enum class MouseButton;
struct SDL_TouchFingerEvent;

/// Enumeration that describes current state of gesture recognition
enum class TouchState
{
	// special state that allows no transitions
	// used when player selects "relative mode" in Launcher
	// in this mode touchscreen acts like touchpad, moving cursor at certains speed
	// and generates events for positions below cursor instead of positions below touch events
	RELATIVE_MODE,

	// no active touch events
	// DOWN -> transition to TAP_DOWN_SHORT
	// MOTION / UP -> not expected
	IDLE,

	// single finger is touching the screen for a short time
	// DOWN -> transition to TAP_DOWN_DOUBLE
	// MOTION -> transition to TAP_DOWN_PANNING
	// UP -> transition to IDLE, emit onLeftClickDown and onLeftClickUp
	// on timer -> transition to TAP_DOWN_LONG, emit onRightClickDown event
	TAP_DOWN_SHORT,

	// single finger is moving across screen
	// DOWN -> transition to TAP_DOWN_DOUBLE
	// MOTION -> emit panning event
	// UP -> transition to IDLE
	TAP_DOWN_PANNING,

	// two fingers are touching the screen
	// DOWN -> ??? how to handle 3rd finger? Ignore?
	// MOTION -> emit pinch event
	// UP -> transition to TAP_DOWN
	TAP_DOWN_DOUBLE,

	// single finger is down for long period of time
	// DOWN -> ignored
	// MOTION -> ignored
	// UP -> transition to TAP_DOWN_LONG_AWAIT
	TAP_DOWN_LONG,

	// right-click popup is active, waiting for new tap to hide popup
	// DOWN -> ignored
	// MOTION -> ignored
	// UP -> transition to IDLE, generate onRightClickUp() event
	TAP_DOWN_LONG_AWAIT,


	// Possible transitions:
	//                               -> DOUBLE
	//                    -> PANNING -> IDLE
	// IDLE -> DOWN_SHORT -> IDLE
	//                    -> LONG -> IDLE
	//                    -> DOUBLE -> PANNING
	//                              -> IDLE
};

struct TouchInputParameters
{
	/// Speed factor of mouse pointer when relative mode is used
	double relativeModeSpeedFactor = 1.0;

	/// tap for period longer than specified here will be qualified as "long tap", triggering corresponding gesture
	uint32_t longTouchTimeMilliseconds = 750;

	/// moving finger for distance larger than specified will be qualified as panning gesture instead of long press
	uint32_t panningSensitivityThreshold = 10;

	/// gesture will be qualified as pinch if distance between fingers is at least specified here
	uint32_t pinchSensitivityThreshold = 10;

	bool useRelativeMode = false;
};

/// Class that handles touchscreen input from SDL events
class InputSourceTouch
{
	TouchInputParameters params;
	TouchState state;
	uint32_t lastTapTimeTicks;
	Point lastTapPosition;

	Point convertTouchToMouse(const SDL_TouchFingerEvent & current);
	Point convertTouchToMouse(float x, float y);

	void emitPanningEvent(const SDL_TouchFingerEvent & tfinger);
	void emitPinchEvent(const SDL_TouchFingerEvent & tfinger);

public:
	InputSourceTouch();

	void handleEventFingerMotion(const SDL_TouchFingerEvent & current);
	void handleEventFingerDown(const SDL_TouchFingerEvent & current);
	void handleEventFingerUp(const SDL_TouchFingerEvent & current);

	void handleUpdate();

	bool hasTouchInputDevice() const;
	bool isMouseButtonPressed(MouseButton button) const;
};
