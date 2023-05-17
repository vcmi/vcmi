/*
 * CGuiHandler.h, part of VCMI engine
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

class InterfaceEventDispatcher
{
	using CIntObjectList = std::list<AEventsReceiver *>;

	//active GUI elements (listening for events
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
	void allowEventHandling(bool enable);

	void handleElementActivate(AEventsReceiver * elem, ui16 activityFlag);
	void handleElementDeActivate(AEventsReceiver * elem, ui16 activityFlag);

	void dispatchTimer(uint32_t msPassed);

	void dispatchShortcutPressed(const std::vector<EShortcut> & shortcuts);
	void dispatchShortcutReleased(const std::vector<EShortcut> & shortcuts);

	void dispatchMouseButtonPressed(const MouseButton & button, const Point & position);
	void dispatchMouseButtonReleased(const MouseButton & button, const Point & position);
	void dispatchMouseScrolled(const Point & distance, const Point & position);
	void dispatchMouseDoubleClick(const Point & position);
	void dispatchMouseMoved(const Point & position);

	void dispatchTextInput(const std::string & text);
	void dispatchTextEditing(const std::string & text);
};
