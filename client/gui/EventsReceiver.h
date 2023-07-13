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
VCMI_LIB_NAMESPACE_END

class EventDispatcher;
enum class EShortcut;

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

public:
	virtual void clickPressed(const Point & cursorPosition) {}
	virtual void clickReleased(const Point & cursorPosition) {}
	virtual void clickCancel(const Point & cursorPosition) {}
	virtual void showPopupWindow(const Point & cursorPosition) {}
	virtual void clickDouble(const Point & cursorPosition) {}

	/// Called when user pans screen by specified distance
	virtual void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) {}

	/// Called when user pitches screen, requesting scaling by specified factor
	virtual void gesturePinch(const Point & centerPosition, double lastUpdateFactor) {}

	virtual void wheelScrolled(int distance) {}
	virtual void mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance) {}
	virtual void mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance) {}

	/// Called when UI element hover status changes
	virtual void hover(bool on) {}

	/// Called when UI element gesture status changes
	virtual void gesture(bool on, const Point & initialPosition, const Point & finalPosition) {}

	virtual void textInputed(const std::string & enteredText) {}
	virtual void textEdited(const std::string & enteredText) {}

	virtual void keyPressed(EShortcut key) {}
	virtual void keyReleased(EShortcut key) {}

	virtual void tick(uint32_t msPassed) {}

public:
	AEventsReceiver();
	virtual ~AEventsReceiver() = default;

	/// These are the arguments that can be used to determine what kind of input UI element will receive
	enum
	{
		LCLICK = 1,
		SHOW_POPUP = 2,
		HOVER = 4,
		MOVE = 8,
		KEYBOARD = 16,
		TIME = 32,
		GENERAL = 64,
		WHEEL = 128,
		DOUBLECLICK = 256,
		TEXTINPUT = 512,
		GESTURE = 1024,
		DRAG = 2048,
	};

	/// Returns true if element is currently hovered by mouse
	bool isHovered() const;

	/// Returns true if panning/swiping gesture is currently active
	bool isGesturing() const;

	/// Returns true if element is currently active and may receive events
	bool isActive() const;

	/// Returns true if left mouse button was pressed when inside this element
	bool isMouseLeftButtonPressed() const;
};
