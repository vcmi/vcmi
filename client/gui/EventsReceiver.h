/*
 * EventsReceiver.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class Point;
class Rect;
VCMI_LIB_NAMESPACE_END

class EventDispatcher;
enum class EShortcut;
enum class InputMode;

/// Class that is capable of subscribing and receiving input events
/// Acts as base class for all UI elements
class AEventsReceiver
{
	friend class EventDispatcher;

	ui16 activeState;
	bool hoveredState;
	bool panningState;
	bool mouseClickedState;

protected:
	/// Activates particular events for this UI element. Uses unnamed enum from this class
	void activateEvents(ui16 what);
	/// Deactivates particular events for this UI element. Uses unnamed enum from this class
	void deactivateEvents(ui16 what);

	/// allows capturing key input so it will be delivered only to this element
	virtual bool captureThisKey(EShortcut key) = 0;

	/// If true, event of selected type in selected position will be processed by this element
	virtual bool receiveEvent(const Point & position, int eventType) const= 0;

	virtual const Rect & getPosition() const= 0;

public:
	virtual void clickPressed(const Point & cursorPosition) {}
	virtual void clickReleased(const Point & cursorPosition) {}
	virtual void clickPressed(const Point & cursorPosition, bool lastActivated);
	virtual void clickReleased(const Point & cursorPosition, bool lastActivated);
	virtual void clickCancel(const Point & cursorPosition) {}
	virtual void showPopupWindow(const Point & cursorPosition) {}
	virtual void closePopupWindow(bool alreadyClosed) {}
	virtual void clickDouble(const Point & cursorPosition) {}
	virtual void notFocusedClick() {};

	/// Called when user pans screen by specified distance
	virtual void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) {}

	/// Called when user pitches screen, requesting scaling by specified factor
	virtual void gesturePinch(const Point & centerPosition, double lastUpdateFactor) {}

	virtual void wheelScrolled(int distance) {}
	virtual void mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance) {}
	virtual void mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance) {}
	virtual void mouseDraggedPopup(const Point & cursorPosition, const Point & lastUpdateDistance) {}

	/// Called when UI element hover status changes
	virtual void hover(bool on) {}

	/// Called when UI element gesture status changes
	virtual void gesture(bool on, const Point & initialPosition, const Point & finalPosition) {}

	virtual void textInputted(const std::string & enteredText) {}
	virtual void textEdited(const std::string & enteredText) {}

	virtual void keyPressed(EShortcut key) {}
	virtual void keyReleased(EShortcut key) {}

	virtual void keyPressed(const std::string & keyName) {}
	virtual void keyReleased(const std::string & keyName) {}

	virtual void tick(uint32_t msPassed) {}

	virtual void inputModeChanged(InputMode modi) {}

public:
	AEventsReceiver();
	virtual ~AEventsReceiver() = default;

	/// These are the arguments that can be used to determine what kind of input UI element will receive
	enum
	{
		LCLICK              = 1 <<  0,
		SHOW_POPUP          = 1 <<  1,
		HOVER               = 1 <<  2,
		MOVE                = 1 <<  3,
		KEYBOARD            = 1 <<  4,
		TIME                = 1 <<  5,
		GENERAL             = 1 <<  6,
		WHEEL               = 1 <<  7,
		DOUBLECLICK         = 1 <<  8,
		TEXTINPUT           = 1 <<  9,
		GESTURE             = 1 << 10,
		DRAG                = 1 << 11,
		INPUT_MODE_CHANGE   = 1 << 12,
		DRAG_POPUP          = 1 << 13,
		KEY_NAME            = 1 << 14
	};

	/// Returns true if element is currently hovered by mouse
	bool isHovered() const;

	/// Returns true if panning/swiping gesture is currently active
	bool isGesturing() const;

	/// Returns true if element is currently active and may receive events
	bool isActive() const;
};
