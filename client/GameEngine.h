/*
 * GameEngine.h, part of VCMI engine
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
class AsyncRunner;
class Rect;
VCMI_LIB_NAMESPACE_END

enum class MouseButton;
class ShortcutHandler;
class FramerateManager;
class IStatusBar;
class CIntObject;
class IGameEngineUser;
class IRenderHandler;
class IScreenHandler;
class WindowHandler;
class EventDispatcher;
class InputHandler;
class ISoundPlayer;
class IMusicPlayer;
class CursorHandler;
class IVideoPlayer;

class GameEngine
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

	std::unique_ptr<ISoundPlayer> soundPlayerInstance;
	std::unique_ptr<IMusicPlayer> musicPlayerInstance;
	std::unique_ptr<CursorHandler> cursorHandlerInstance;
	std::unique_ptr<IVideoPlayer> videoPlayerInstance;
	std::unique_ptr<AsyncRunner> asyncTasks;

	IGameEngineUser *engineUser = nullptr;

	int maxPerformanceOverlayTextWidth = 0;

	void updateFrame();
	void handleEvents(); //takes events from queue and calls interested objects
	void drawPerformanceOverlay(); // draws box with additional infos (e.g. fps)

public:
	std::mutex interfaceMutex;

	/// returns current position of mouse cursor, relative to vcmi window
	const Point & getCursorPosition() const;

	ShortcutHandler & shortcuts();
	FramerateManager & framerate();
	EventDispatcher & events();
	InputHandler & input();

	AsyncRunner & async() { return *asyncTasks; }
	IGameEngineUser & user() { return *engineUser; }
	ISoundPlayer & sound() { return *soundPlayerInstance; }
	IMusicPlayer & music() { return *musicPlayerInstance; }
	CursorHandler & cursor() { return *cursorHandlerInstance; }
	IVideoPlayer & video() { return *videoPlayerInstance; }

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

	IScreenHandler & screenHandler();
	IRenderHandler & renderHandler();
	WindowHandler & windows();

	/// Returns currently active status bar. Guaranteed to be non-null
	std::shared_ptr<IStatusBar> statusbar();

	/// Set currently active status bar
	void setStatusbar(const std::shared_ptr<IStatusBar> &);

	/// Sets engine user that is used as target of callback for events received by engine
	void setEngineUser(IGameEngineUser * user);

	bool captureChildren; //all newly created objects will get their parents from stack and will be added to parents children list
	std::list<CIntObject *> createdObj; //stack of objs being created

	GameEngine();
	~GameEngine();

	/// Performs main game loop till game shutdown
	/// This method never returns, to abort main loop throw GameShutdownException
	[[noreturn]] void mainLoop();

	/// called whenever SDL_WINDOWEVENT_RESTORED is reported or the user selects a different resolution, requiring to center/resize all windows
	void onScreenResize(bool resolutionChanged);

	/// Simulate mouse movement to force refresh UI state that updates on mouse move
	void fakeMouseMove();

	/// Returns true for calls made from main (GUI) thread, false othervice
	bool amIGuiThread();

	/// Calls provided functor in main thread on next execution frame
	void dispatchMainThread(const std::function<void()> & functor);
};

extern std::unique_ptr<GameEngine> ENGINE; //global gui handler
