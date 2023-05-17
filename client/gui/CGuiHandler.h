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
class FramerateManager;
class IStatusBar;
class CIntObject;
class IUpdateable;
class IShowActivatable;
class IScreenHandler;
class WindowHandler;
class InterfaceEventDispatcher;

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

// Handles GUI logic and drawing
class CGuiHandler
{
private:
	/// Fake no-op version status bar, for use in windows that have no status bar
	std::shared_ptr<IStatusBar> fakeStatusBar;

	/// Status bar of current window, if any. Uses weak_ptr to allow potential hanging reference after owned window has been deleted
	std::weak_ptr<IStatusBar> currentStatusBar;

	Point cursorPosition;
	uint32_t mouseButtonsMask;

	std::unique_ptr<ShortcutHandler> shortcutsHandlerInstance;
	std::unique_ptr<WindowHandler> windowHandlerInstance;

	std::unique_ptr<IScreenHandler> screenHandlerInstance;
	std::unique_ptr<FramerateManager> framerateManagerInstance;
	std::unique_ptr<InterfaceEventDispatcher> eventDispatcherInstance;

	void handleCurrentEvent(SDL_Event &current);
	void convertTouchToMouse(SDL_Event * current);
	void fakeMoveCursor(float dx, float dy);
	void fakeMouseButtonEventRelativeMode(bool down, bool right);

	void handleEventKeyDown(SDL_Event & current);
	void handleEventKeyUp(SDL_Event & current);
	void handleEventMouseMotion(SDL_Event & current);
	void handleEventMouseButtonDown(SDL_Event & current);
	void handleEventMouseWheel(SDL_Event & current);
	void handleEventTextInput(SDL_Event & current);
	void handleEventTextEditing(SDL_Event & current);
	void handleEventMouseButtonUp(SDL_Event & current);
	void handleEventFingerMotion(SDL_Event & current);
	void handleEventFingerDown(SDL_Event & current);
	void handleEventFingerUp(SDL_Event & current);

public:

	/// returns current position of mouse cursor, relative to vcmi window
	const Point & getCursorPosition() const;

	ShortcutHandler & shortcutsHandler();
	FramerateManager & framerateManager();
	InterfaceEventDispatcher & eventDispatcher();

	/// Returns current logical screen dimensions
	/// May not match size of window if user has UI scaling different from 100%
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

	IScreenHandler & screenHandler();

	WindowHandler & windows();

	/// Returns currently active status bar. Guaranteed to be non-null
	std::shared_ptr<IStatusBar> statusbar();

	/// Set currently active status bar
	void setStatusbar(std::shared_ptr<IStatusBar>);

	IUpdateable *curInt;

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

	/// called whenever user selects different resolution, requiring to center/resize all windows
	void onScreenResize();

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
