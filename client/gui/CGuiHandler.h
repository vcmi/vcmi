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

#include "UiMutex.h"

VCMI_LIB_NAMESPACE_BEGIN
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
class IRenderHandler;
class IScreenHandler;
class WindowHandler;
class EventDispatcher;
class InputHandler;

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
	std::unique_ptr<IRenderHandler> renderHandlerInstance;
	std::unique_ptr<FramerateManager> framerateManagerInstance;
	std::unique_ptr<EventDispatcher> eventDispatcherInstance;
	std::unique_ptr<InputHandler> inputHandlerInstance;

public:
	ui_mutex interfaceMutex;

	/// returns current position of mouse cursor, relative to vcmi window
	const Point & getCursorPosition() const;

	ShortcutHandler & shortcuts();
	FramerateManager & framerate();
	EventDispatcher & events();
	InputHandler & input();

	/// Returns current logical screen dimensions
	/// May not match size of window if user has UI scaling different from 100%
	Point screenDimensions() const;

	/// returns true if Alt is currently pressed down
	bool isKeyboardAltDown() const;
	/// returns true if Ctrl is currently pressed down
	/// on Apple system, this also tests for Cmd key
	/// For use with keyboard-based events
	bool isKeyboardCtrlDown() const;
	/// on Apple systems, returns true if Cmd key is pressed
	/// on other systems, returns true if Ctrl is pressed
	/// /// For use with mouse-based events
	bool isKeyboardCmdDown() const;
	/// returns true if Shift is currently pressed down
	bool isKeyboardShiftDown() const;

	void startTextInput(const Rect & where);
	void stopTextInput();

	IScreenHandler & screenHandler();
	IRenderHandler & renderHandler();
	WindowHandler & windows();

	/// Returns currently active status bar. Guaranteed to be non-null
	std::shared_ptr<IStatusBar> statusbar();

	/// Set currently active status bar
	void setStatusbar(std::shared_ptr<IStatusBar>);

	IUpdateable *curInt;

	bool captureChildren; //all newly created objects will get their parents from stack and will be added to parents children list
	std::list<CIntObject *> createdObj; //stack of objs being created

	CGuiHandler();
	~CGuiHandler();

	void init();
	void renderFrame();

	/// called whenever SDL_WINDOWEVENT_RESTORED is reported or the user selects a different resolution, requiring to center/resize all windows
	void onScreenResize(bool resolutionChanged);

	void handleEvents(); //takes events from queue and calls interested objects
	void fakeMouseMove();
	void drawFPSCounter(); // draws the FPS to the upper left corner of the screen

	bool amIGuiThread();

	/// Calls provided functor in main thread on next execution frame
	void dispatchMainThread(const std::function<void()> & functor);
};

extern CGuiHandler GH; //global gui handler
