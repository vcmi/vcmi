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
template <typename T> struct CondSh;
class Point;
class Rect;
VCMI_LIB_NAMESPACE_END

enum class MouseButton;
class ShortcutHandler;
class FramerateManager;
class IStatusBar;
class CIntObject;
class IUpdateable;
class IShowActivatable;
class IScreenHandler;
class WindowHandler;
class EventDispatcher;
class InputHandler;

// TODO: event handling need refactoring. Perhaps convert into delayed function call?
enum class EUserEvent
{
	RETURN_TO_MAIN_MENU,
	RESTART_GAME,
	RETURN_TO_MENU_LOAD,
	FULLSCREEN_TOGGLED,
	CAMPAIGN_START_SCENARIO,
	FORCE_QUIT,
};

// Handles GUI logic and drawing
class CGuiHandler
{
private:
	/// Fake no-op version status bar, for use in windows that have no status bar
	std::shared_ptr<IStatusBar> fakeStatusBar;

	/// Status bar of current window, if any. Uses weak_ptr to allow potential hanging reference after owned window has been deleted
	std::weak_ptr<IStatusBar> currentStatusBar;

	std::unique_ptr<ShortcutHandler> shortcutsHandlerInstance;
	std::unique_ptr<WindowHandler> windowHandlerInstance;

	std::unique_ptr<IScreenHandler> screenHandlerInstance;
	std::unique_ptr<FramerateManager> framerateManagerInstance;
	std::unique_ptr<EventDispatcher> eventDispatcherInstance;
	std::unique_ptr<InputHandler> inputHandlerInstance;

public:
	/// returns current position of mouse cursor, relative to vcmi window
	const Point & getCursorPosition() const;

	ShortcutHandler & shortcuts();
	FramerateManager & framerate();
	EventDispatcher & events();
	InputHandler & input();

	/// Returns current logical screen dimensions
	/// May not match size of window if user has UI scaling different from 100%
	Point screenDimensions() const;

	/// returns true if specified mouse button is pressed
	bool isMouseButtonPressed(MouseButton button) const;

	/// returns true if chosen keyboard key is currently pressed down
	bool isKeyboardAltDown() const;
	bool isKeyboardCtrlDown() const;
	bool isKeyboardShiftDown() const;

	void startTextInput(const Rect & where);
	void stopTextInput();

	IScreenHandler & screenHandler();

	WindowHandler & windows();

	/// Returns currently active status bar. Guaranteed to be non-null
	std::shared_ptr<IStatusBar> statusbar();

	/// Set currently active status bar
	void setStatusbar(std::shared_ptr<IStatusBar>);

	IUpdateable *curInt;

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

	bool amIGuiThread();
	void pushUserEvent(EUserEvent usercode);
	void pushUserEvent(EUserEvent usercode, void * userdata);

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
