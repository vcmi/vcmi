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

class AEventsReceiver
{
	friend class EventDispatcher;

	ui16 activeState;
	std::map<MouseButton, bool> currentMouseState;

	bool hoveredState; //for determining if object is hovered
	bool strongInterestState; //if true - report all mouse movements, if not - only when hovered

	void click(MouseButton btn, tribool down, bool previousState);
protected:
	void setMoveEventStrongInterest(bool on);

	void activateEvents(ui16 what);
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

	// These are the arguments that can be used to determine what kind of input the CIntObject will receive
	enum {LCLICK=1, RCLICK=2, HOVER=4, MOVE=8, KEYBOARD=16, TIME=32, GENERAL=64, WHEEL=128, DOUBLECLICK=256, TEXTINPUT=512, MCLICK=1024, ALL=0xffff};

	bool isHovered() const;
	bool isActive() const;
	bool isActive(int flags) const;
	bool isMouseButtonPressed(MouseButton btn) const;
};
