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

#include "MouseButton.h"
#include "../../lib/Point.h"

VCMI_LIB_NAMESPACE_BEGIN

template <typename T> struct CondSh;
class Rect;

VCMI_LIB_NAMESPACE_END

union SDL_Event;
struct SDL_MouseMotionEvent;

class ShortcutHandler;
class CFramerateManager;
class IStatusBar;
class CIntObject;
class IUpdateable;
class IShowActivatable;
class IShowable;

// TODO: event handling need refactoring
enum class EUserEvent
{
	/*CHANGE_SCREEN_RESOLUTION = 1,*/
	RETURN_TO_MAIN_MENU = 2,
	//STOP_CLIENT = 3,
	RESTART_GAME = 4,
	RETURN_TO_MENU_LOAD,
	FULLSCREEN_TOGGLED,
	CAMPAIGN_START_SCENARIO,
	FORCE_QUIT, //quit client without question
};

// A fps manager which holds game updates at a constant rate
class CFramerateManager
{
private:
	double rateticks;
	ui32 lastticks;
	ui32 timeElapsed;
	int rate;
	int fps; // the actual fps value
	ui32 accumulatedTime;
	ui32 accumulatedFrames;

public:
	CFramerateManager(); // initializes the manager with a given fps rate
	void init(int newRate); // needs to be called directly before the main game loop to reset the internal timer
	void framerateDelay(); // needs to be called every game update cycle
	ui32 getElapsedMilliseconds() const {return this->timeElapsed;}
	ui32 getFrameNumber() const { return accumulatedFrames; }
	ui32 getFramerate() const { return fps; };
};

// Handles GUI logic and drawing
class CGuiHandler
{
public:
	CFramerateManager * mainFPSmng; //to keep const framerate
	std::list<std::shared_ptr<IShowActivatable>> listInt; //list of interfaces - front=foreground; back = background (includes adventure map, window interfaces, all kind of active dialogs, and so on)
	std::shared_ptr<IStatusBar> statusbar;

private:
	Point cursorPosition;
	uint32_t mouseButtonsMask;

	std::vector<std::shared_ptr<IShowActivatable>> disposed;

	std::unique_ptr<ShortcutHandler> shortcutsHandler;

	std::atomic<bool> continueEventHandling;
	using CIntObjectList = std::list<CIntObject *>;

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


	void handleMouseButtonClick(CIntObjectList & interestedObjs, MouseButton btn, bool isPressed);
	void processLists(const ui16 activityFlag, std::function<void (std::list<CIntObject*> *)> cb);
	void handleCurrentEvent(SDL_Event &current);
	void handleMouseMotion(const SDL_Event & current);
	void handleMoveInterested( const SDL_MouseMotionEvent & motion );
	void convertTouchToMouse(SDL_Event * current);
	void fakeMoveCursor(float dx, float dy);
	void fakeMouseButtonEventRelativeMode(bool down, bool right);

public:
	void handleElementActivate(CIntObject * elem, ui16 activityFlag);
	void handleElementDeActivate(CIntObject * elem, ui16 activityFlag);
public:
	//objs to blit
	std::vector<std::shared_ptr<IShowActivatable>> objsToBlit;
	/// returns current position of mouse cursor, relative to vcmi window
	const Point & getCursorPosition() const;

	ShortcutHandler & getShortcutsHandler();

	Point screenDimensions() const;

	/// returns true if at least one mouse button is pressed
	bool isMouseButtonPressed() const;

	/// returns true if specified mouse button is pressed
	bool isMouseButtonPressed(MouseButton button) const;

	/// returns true if chosen keyboard key is currently pressed down
	bool isKeyboardAltDown() const;
	bool isKeyboardCtrlDown() const;
	bool isKeyboardShiftDown() const;

	void startTextInput(const Rect & where);
	void stopTextInput();

	/// moves mouse pointer into specified position inside vcmi window
	void moveCursorToPosition(const Point & position);

	IUpdateable *curInt;

	Point lastClick;
	unsigned lastClickTime;
	bool multifinger;
	bool isPointerRelativeMode;
	float pointerSpeedMultiplier;

	ui8 defActionsDef; //default auto actions
	bool captureChildren; //all newly created objects will get their parents from stack and will be added to parents children list
	std::list<CIntObject *> createdObj; //stack of objs being created

	CGuiHandler();
	~CGuiHandler();

	void init();
	void renderFrame();

	void totalRedraw(); //forces total redraw (using showAll), sets a flag, method gets called at the end of the rendering
	void simpleRedraw(); //update only top interface and draw background from buffer, sets a flag, method gets called at the end of the rendering

	void pushInt(std::shared_ptr<IShowActivatable> newInt); //deactivate old top interface, activates this one and pushes to the top
	template <typename T, typename ... Args>
	void pushIntT(Args && ... args)
	{
		auto newInt = std::make_shared<T>(std::forward<Args>(args)...);
		pushInt(newInt);
	}

	void popInts(int howMany); //pops one or more interfaces - deactivates top, deletes and removes given number of interfaces, activates new front

	void popInt(std::shared_ptr<IShowActivatable> top); //removes given interface from the top and activates next

	std::shared_ptr<IShowActivatable> topInt(); //returns top interface

	void updateTime(); //handles timeInterested
	void handleEvents(); //takes events from queue and calls interested objects
	void fakeMouseMove();
	void breakEventHandling(); //current event won't be propagated anymore
	void drawFPSCounter(); // draws the FPS to the upper left corner of the screen

	static bool amIGuiThread();
	static void pushUserEvent(EUserEvent usercode);
	static void pushUserEvent(EUserEvent usercode, void * userdata);

	CondSh<bool> * terminate_cond; // confirm termination
};

extern CGuiHandler GH; //global gui handler

struct SObjectConstruction
{
	CIntObject *myObj;
	SObjectConstruction(CIntObject *obj);
	~SObjectConstruction();
};

struct SSetCaptureState
{
	bool previousCapture;
	ui8 prevActions;
	SSetCaptureState(bool allow, ui8 actions);
	~SSetCaptureState();
};

#define OBJ_CONSTRUCTION SObjectConstruction obj__i(this)
#define OBJ_CONSTRUCTION_TARGETED(obj) SObjectConstruction obj__i(obj)
#define OBJECT_CONSTRUCTION_CAPTURING(actions) defActions = actions; SSetCaptureState obj__i1(true, actions); SObjectConstruction obj__i(this)
#define OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(actions) SSetCaptureState obj__i1(true, actions); SObjectConstruction obj__i(this)

#define OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE defActions = 255 - DISPOSE; SSetCaptureState obj__i1(true, 255 - DISPOSE); SObjectConstruction obj__i(this)
