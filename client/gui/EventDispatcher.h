/*
 * EventDispatcher.h, part of VCMI engine
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

class AEventsReceiver;
enum class MouseButton;
enum class EShortcut;

/// Class that receives events from event producers and dispatches it to UI elements that are interested in this event
class EventDispatcher
{
	using CIntObjectList = std::list<AEventsReceiver *>;

	/// list of UI elements that are interested in particular event
	CIntObjectList lclickable;
	CIntObjectList rclickable;
	CIntObjectList mclickable;
	CIntObjectList hoverable;
	CIntObjectList keyinterested;
	CIntObjectList motioninterested;
	CIntObjectList timeinterested;
	CIntObjectList wheelInterested;
	CIntObjectList doubleClickInterested;
	CIntObjectList textInterested;

	std::atomic<bool> eventHandlingAllowed = true;

	CIntObjectList & getListForMouseButton(MouseButton button);

	void handleMouseButtonClick(CIntObjectList & interestedObjs, MouseButton btn, bool isPressed);

	void processList(const ui16 mask, const ui16 flag, CIntObjectList * lst, std::function<void(CIntObjectList *)> cb);
	void processLists(ui16 activityFlag, std::function<void(CIntObjectList *)> cb);

public:
	/// allows to interrupt event handling and abort any subsequent event processing
	void allowEventHandling(bool enable);

	/// add specified UI element as interested. Uses unnamed enum from AEventsReceiver for activity flags
	void handleElementActivate(AEventsReceiver * elem, ui16 activityFlag);

	/// removes specified UI element as interested for specified activities
	void handleElementDeActivate(AEventsReceiver * elem, ui16 activityFlag);

	/// Regular timer event
	void dispatchTimer(uint32_t msPassed);

	/// Shortcut events (e.g. keyboard keys)
	void dispatchShortcutPressed(const std::vector<EShortcut> & shortcuts);
	void dispatchShortcutReleased(const std::vector<EShortcut> & shortcuts);

	/// Mouse events
	void dispatchMouseButtonPressed(const MouseButton & button, const Point & position);
	void dispatchMouseButtonReleased(const MouseButton & button, const Point & position);
	void dispatchMouseScrolled(const Point & distance, const Point & position);
	void dispatchMouseDoubleClick(const Point & position);
	void dispatchMouseMoved(const Point & position);

	/// Text input events
	void dispatchTextInput(const std::string & text);
	void dispatchTextEditing(const std::string & text);
};
