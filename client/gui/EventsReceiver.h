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
enum class MouseButton;
enum class EShortcut;
using boost::logic::tribool;

/// Class that is capable of subscribing and receiving input events
/// Acts as base class for all UI elements
class AEventsReceiver
{
	friend class EventDispatcher;

	ui16 activeState;
	bool hoveredState;
	bool strongInterestState;
	std::map<MouseButton, bool> currentMouseState;

	void click(MouseButton btn, tribool down, bool previousState);
protected:

	/// If set, UI element will receive all mouse movement events, even those outside this element
	void setMoveEventStrongInterest(bool on);

	/// Activates particular events for this UI element. Uses unnamed enum from this class
	void activateEvents(ui16 what);
	/// Deactivates particular events for this UI element. Uses unnamed enum from this class
	void deactivateEvents(ui16 what);

	virtual void clickLeft(tribool down, bool previousState) {}
	virtual void clickRight(tribool down, bool previousState) {}
	virtual void clickMiddle(tribool down, bool previousState) {}

	virtual void textInputed(const std::string & enteredText) {}
	virtual void textEdited(const std::string & enteredText) {}

	virtual void tick(uint32_t msPassed) {}
	virtual void wheelScrolled(bool down, bool in) {}
	virtual void mouseMoved(const Point & cursorPosition) {}
	virtual void hover(bool on) {}
	virtual void onDoubleClick() {}

	virtual void keyPressed(EShortcut key) {}
	virtual void keyReleased(EShortcut key) {}

	virtual bool captureThisKey(EShortcut key) = 0;
	virtual bool isInside(const Point & position) = 0;

public:
	AEventsReceiver();
	virtual ~AEventsReceiver() = default;

	/// These are the arguments that can be used to determine what kind of input UI element will receive
	enum {LCLICK=1, RCLICK=2, HOVER=4, MOVE=8, KEYBOARD=16, TIME=32, GENERAL=64, WHEEL=128, DOUBLECLICK=256, TEXTINPUT=512, MCLICK=1024, ALL=0xffff};

	/// Returns true if element is currently hovered by mouse
	bool isHovered() const;

	/// Returns true if element is currently active and may receive events
	bool isActive() const;

	/// Returns true if particular event(s) is active for this element
	bool isActive(int flags) const;

	/// Returns true if particular mouse button was pressed when inside this element
	bool isMouseButtonPressed(MouseButton btn) const;
};
