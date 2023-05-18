/*
 * CGuiHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGuiHandler.h"
#include "../lib/CondSh.h"

#include "CIntObject.h"
#include "CursorHandler.h"
#include "ShortcutHandler.h"
#include "FramerateManager.h"
#include "WindowHandler.h"
#include "EventDispatcher.h"

#include "../CGameInfo.h"
#include "../render/Colors.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../renderSDL/ScreenHandler.h"
#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../battle/BattleInterface.h"

#include "../../lib/CThreadHelper.h"
#include "../../lib/CConfigHandler.h"

#include <SDL_render.h>
#include <SDL_timer.h>
#include <SDL_events.h>
#include <SDL_keycode.h>

#ifdef VCMI_APPLE
#include <dispatch/dispatch.h>
#endif

#ifdef VCMI_IOS
#include "ios/utils.h"
#endif

CGuiHandler GH;

extern std::queue<SDL_Event> SDLEventsQueue;
extern boost::mutex eventsM;

boost::thread_specific_ptr<bool> inGuiThread;

SObjectConstruction::SObjectConstruction(CIntObject *obj)
:myObj(obj)
{
	GH.createdObj.push_front(obj);
	GH.captureChildren = true;
}

SObjectConstruction::~SObjectConstruction()
{
	assert(GH.createdObj.size());
	assert(GH.createdObj.front() == myObj);
	GH.createdObj.pop_front();
	GH.captureChildren = GH.createdObj.size();
}

SSetCaptureState::SSetCaptureState(bool allow, ui8 actions)
{
	previousCapture = GH.captureChildren;
	GH.captureChildren = false;
	prevActions = GH.defActionsDef;
	GH.defActionsDef = actions;
}

SSetCaptureState::~SSetCaptureState()
{
	GH.captureChildren = previousCapture;
	GH.defActionsDef = prevActions;
}

void CGuiHandler::init()
{
	eventDispatcherInstance = std::make_unique<EventDispatcher>();
	windowHandlerInstance = std::make_unique<WindowHandler>();
	screenHandlerInstance = std::make_unique<ScreenHandler>();
	shortcutsHandlerInstance = std::make_unique<ShortcutHandler>();
	framerateManagerInstance = std::make_unique<FramerateManager>(settings["video"]["targetfps"].Integer());

	isPointerRelativeMode = settings["general"]["userRelativePointer"].Bool();
	pointerSpeedMultiplier = settings["general"]["relativePointerSpeedMultiplier"].Float();
}

void CGuiHandler::handleEvents()
{
	eventDispatcher().dispatchTimer(framerateManager().getElapsedMilliseconds());

	//player interface may want special event handling
	if(nullptr != LOCPLINT && LOCPLINT->capturedAllEvents())
		return;

	boost::unique_lock<boost::mutex> lock(eventsM);
	while(!SDLEventsQueue.empty())
	{
		eventDispatcher().allowEventHandling(true);
		SDL_Event currentEvent = SDLEventsQueue.front();

		if (currentEvent.type == SDL_MOUSEMOTION)
		{
			cursorPosition = Point(currentEvent.motion.x, currentEvent.motion.y);
			mouseButtonsMask = currentEvent.motion.state;
		}
		SDLEventsQueue.pop();

		// In a sequence of mouse motion events, skip all but the last one.
		// This prevents freezes when every motion event takes longer to handle than interval at which
		// the events arrive (like dragging on the minimap in world view, with redraw at every event)
		// so that the events would start piling up faster than they can be processed.
		if ((currentEvent.type == SDL_MOUSEMOTION) && !SDLEventsQueue.empty() && (SDLEventsQueue.front().type == SDL_MOUSEMOTION))
			continue;

		handleCurrentEvent(currentEvent);
	}
}

void CGuiHandler::convertTouchToMouse(SDL_Event * current)
{
	int rLogicalWidth, rLogicalHeight;

	SDL_RenderGetLogicalSize(mainRenderer, &rLogicalWidth, &rLogicalHeight);

	int adjustedMouseY = (int)(current->tfinger.y * rLogicalHeight);
	int adjustedMouseX = (int)(current->tfinger.x * rLogicalWidth);

	current->button.x = adjustedMouseX;
	current->motion.x = adjustedMouseX;
	current->button.y = adjustedMouseY;
	current->motion.y = adjustedMouseY;
}

void CGuiHandler::fakeMoveCursor(float dx, float dy)
{
	int x, y, w, h;

	SDL_Event event;
	SDL_MouseMotionEvent sme = {SDL_MOUSEMOTION, 0, 0, 0, 0, 0, 0, 0, 0};

	sme.state = SDL_GetMouseState(&x, &y);
	SDL_GetWindowSize(mainWindow, &w, &h);

	sme.x = CCS->curh->position().x + (int)(GH.pointerSpeedMultiplier * w * dx);
	sme.y = CCS->curh->position().y + (int)(GH.pointerSpeedMultiplier * h * dy);

	vstd::abetween(sme.x, 0, w);
	vstd::abetween(sme.y, 0, h);

	event.motion = sme;
	SDL_PushEvent(&event);
}

void CGuiHandler::fakeMouseMove()
{
	fakeMoveCursor(0, 0);
}

void CGuiHandler::startTextInput(const Rect & whereInput)
{
#ifdef VCMI_APPLE
	dispatch_async(dispatch_get_main_queue(), ^{
#endif

	// TODO ios: looks like SDL bug actually, try fixing there
	auto renderer = SDL_GetRenderer(mainWindow);
	float scaleX, scaleY;
	SDL_Rect viewport;
	SDL_RenderGetScale(renderer, &scaleX, &scaleY);
	SDL_RenderGetViewport(renderer, &viewport);

#ifdef VCMI_IOS
	const auto nativeScale = iOS_utils::screenScale();
	scaleX /= nativeScale;
	scaleY /= nativeScale;
#endif

	SDL_Rect rectInScreenCoordinates;
	rectInScreenCoordinates.x = (viewport.x + whereInput.x) * scaleX;
	rectInScreenCoordinates.y = (viewport.y + whereInput.y) * scaleY;
	rectInScreenCoordinates.w = whereInput.w * scaleX;
	rectInScreenCoordinates.h = whereInput.h * scaleY;

	SDL_SetTextInputRect(&rectInScreenCoordinates);

	if (SDL_IsTextInputActive() == SDL_FALSE)
	{
		SDL_StartTextInput();
	}

#ifdef VCMI_APPLE
	});
#endif
}

void CGuiHandler::stopTextInput()
{
#ifdef VCMI_APPLE
	dispatch_async(dispatch_get_main_queue(), ^{
#endif

	if (SDL_IsTextInputActive() == SDL_TRUE)
	{
		SDL_StopTextInput();
	}

#ifdef VCMI_APPLE
	});
#endif
}

void CGuiHandler::fakeMouseButtonEventRelativeMode(bool down, bool right)
{
	SDL_Event event;
	SDL_MouseButtonEvent sme = {SDL_MOUSEBUTTONDOWN, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	if(!down)
	{
		sme.type = SDL_MOUSEBUTTONUP;
	}

	sme.button = right ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;

	sme.x = CCS->curh->position().x;
	sme.y = CCS->curh->position().y;

	float xScale, yScale;
	int w, h, rLogicalWidth, rLogicalHeight;

	SDL_GetWindowSize(mainWindow, &w, &h);
	SDL_RenderGetLogicalSize(mainRenderer, &rLogicalWidth, &rLogicalHeight);
	SDL_RenderGetScale(mainRenderer, &xScale, &yScale);

	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	moveCursorToPosition( Point(
		(int)(sme.x * xScale) + (w - rLogicalWidth * xScale) / 2,
		(int)(sme.y * yScale + (h - rLogicalHeight * yScale) / 2)));
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);

	event.button = sme;
	SDL_PushEvent(&event);
}

void CGuiHandler::handleCurrentEvent( SDL_Event & current )
{
	switch (current.type)
	{
		case SDL_KEYDOWN:
			return handleEventKeyDown(current);
		case SDL_KEYUP:
			return handleEventKeyUp(current);
		case SDL_MOUSEMOTION:
			return handleEventMouseMotion(current);
		case SDL_MOUSEBUTTONDOWN:
			return handleEventMouseButtonDown(current);
		case SDL_MOUSEWHEEL:
			return handleEventMouseWheel(current);
		case SDL_TEXTINPUT:
			return handleEventTextInput(current);
		case SDL_TEXTEDITING:
			return handleEventTextEditing(current);
		case SDL_MOUSEBUTTONUP:
			return handleEventMouseButtonUp(current);
		case SDL_FINGERMOTION:
			return handleEventFingerMotion(current);
		case SDL_FINGERDOWN:
			return handleEventFingerDown(current);
		case SDL_FINGERUP:
			return handleEventFingerUp(current);
	}
}

void CGuiHandler::handleEventKeyDown(SDL_Event & current)
{
	SDL_KeyboardEvent key = current.key;

	if(key.repeat != 0)
		return; // ignore periodic event resends

	assert(key.state == SDL_PRESSED);

	if(current.type == SDL_KEYDOWN && key.keysym.sym >= SDLK_F1 && key.keysym.sym <= SDLK_F15 && settings["session"]["spectate"].Bool())
	{
		//TODO: we need some central place for all interface-independent hotkeys
		Settings s = settings.write["session"];
		switch(key.keysym.sym)
		{
			case SDLK_F5:
				if(settings["session"]["spectate-locked-pim"].Bool())
					LOCPLINT->pim->unlock();
				else
					LOCPLINT->pim->lock();
				s["spectate-locked-pim"].Bool() = !settings["session"]["spectate-locked-pim"].Bool();
				break;

			case SDLK_F6:
				s["spectate-ignore-hero"].Bool() = !settings["session"]["spectate-ignore-hero"].Bool();
				break;

			case SDLK_F7:
				s["spectate-skip-battle"].Bool() = !settings["session"]["spectate-skip-battle"].Bool();
				break;

			case SDLK_F8:
				s["spectate-skip-battle-result"].Bool() = !settings["session"]["spectate-skip-battle-result"].Bool();
				break;

			default:
				break;
		}
		return;
	}

	auto shortcutsVector = shortcutsHandler().translateKeycode(key.keysym.sym);

	eventDispatcher().dispatchShortcutPressed(shortcutsVector);
}

void CGuiHandler::handleEventKeyUp(SDL_Event & current)
{
	SDL_KeyboardEvent key = current.key;

	if(key.repeat != 0)
		return; // ignore periodic event resends

	assert(key.state == SDL_RELEASED);

	auto shortcutsVector = shortcutsHandler().translateKeycode(key.keysym.sym);

	eventDispatcher().dispatchShortcutReleased(shortcutsVector);
}

void CGuiHandler::handleEventMouseMotion(SDL_Event & current)
{
	eventDispatcher().dispatchMouseMoved(Point(current.motion.x, current.motion.y));
}

void CGuiHandler::handleEventMouseButtonDown(SDL_Event & current)
{
	switch(current.button.button)
	{
		case SDL_BUTTON_LEFT:
			if (current.button.clicks > 1)
				eventDispatcher().dispatchMouseDoubleClick(Point(current.button.x, current.button.y));
			else
				eventDispatcher().dispatchMouseButtonPressed(MouseButton::LEFT, Point(current.button.x, current.button.y));
			break;
		case SDL_BUTTON_RIGHT:
			eventDispatcher().dispatchMouseButtonPressed(MouseButton::RIGHT, Point(current.button.x, current.button.y));
			break;
		case SDL_BUTTON_MIDDLE:
			eventDispatcher().dispatchMouseButtonPressed(MouseButton::MIDDLE, Point(current.button.x, current.button.y));
			break;
	}
}

void CGuiHandler::handleEventMouseWheel(SDL_Event & current)
{
	// SDL doesn't have the proper values for mouse positions on SDL_MOUSEWHEEL, refetch them
	int x = 0, y = 0;
	SDL_GetMouseState(&x, &y);

	eventDispatcher().dispatchMouseScrolled(Point(current.wheel.x, current.wheel.y), Point(x, y));
}

void CGuiHandler::handleEventTextInput(SDL_Event & current)
{
	eventDispatcher().dispatchTextInput(current.text.text);
}

void CGuiHandler::handleEventTextEditing(SDL_Event & current)
{
	eventDispatcher().dispatchTextEditing(current.text.text);
}

void CGuiHandler::handleEventMouseButtonUp(SDL_Event & current)
{
	if(!multifinger)
	{
		switch(current.button.button)
		{
			case SDL_BUTTON_LEFT:
				eventDispatcher().dispatchMouseButtonReleased(MouseButton::LEFT, Point(current.button.x, current.button.y));
				break;
			case SDL_BUTTON_RIGHT:
				eventDispatcher().dispatchMouseButtonReleased(MouseButton::RIGHT, Point(current.button.x, current.button.y));
				break;
			case SDL_BUTTON_MIDDLE:
				eventDispatcher().dispatchMouseButtonReleased(MouseButton::MIDDLE, Point(current.button.x, current.button.y));
				break;
		}
	}
}

void CGuiHandler::handleEventFingerMotion(SDL_Event & current)
{
	if(isPointerRelativeMode)
	{
		fakeMoveCursor(current.tfinger.dx, current.tfinger.dy);
	}
}
	
void CGuiHandler::handleEventFingerDown(SDL_Event & current)
{
	auto fingerCount = SDL_GetNumTouchFingers(current.tfinger.touchId);

	multifinger = fingerCount > 1;

	if(isPointerRelativeMode)
	{
		if(current.tfinger.x > 0.5)
		{
			bool isRightClick = current.tfinger.y < 0.5;

			fakeMouseButtonEventRelativeMode(true, isRightClick);
		}
	}
#ifndef VCMI_IOS
	else if(fingerCount == 2)
	{
		convertTouchToMouse(&current);

		eventDispatcher().dispatchMouseMoved(Point(current.button.x, current.button.y));
		eventDispatcher().dispatchMouseButtonPressed(MouseButton::RIGHT, Point(current.button.x, current.button.y));
	}
#endif //VCMI_IOS
}

void CGuiHandler::handleEventFingerUp(SDL_Event & current)
{
#ifndef VCMI_IOS
	auto fingerCount = SDL_GetNumTouchFingers(current.tfinger.touchId);
#endif //VCMI_IOS

	if(isPointerRelativeMode)
	{
		if(current.tfinger.x > 0.5)
		{
			bool isRightClick = current.tfinger.y < 0.5;

			fakeMouseButtonEventRelativeMode(false, isRightClick);
		}
	}
#ifndef VCMI_IOS
	else if(multifinger)
	{
		convertTouchToMouse(&current);
		eventDispatcher().dispatchMouseMoved(Point(current.button.x, current.button.y));
		eventDispatcher().dispatchMouseButtonReleased(MouseButton::RIGHT, Point(current.button.x, current.button.y));
		multifinger = fingerCount != 0;
	}
#endif //VCMI_IOS
}

void CGuiHandler::renderFrame()
{
	// Updating GUI requires locking pim mutex (that protects screen and GUI state).
	// During game:
	// When ending the game, the pim mutex might be hold by other thread,
	// that will notify us about the ending game by setting terminate_cond flag.
	//in PreGame terminate_cond stay false

	bool acquiredTheLockOnPim = false; //for tracking whether pim mutex locking succeeded
	while(!terminate_cond->get() && !(acquiredTheLockOnPim = CPlayerInterface::pim->try_lock())) //try acquiring long until it succeeds or we are told to terminate
		boost::this_thread::sleep(boost::posix_time::milliseconds(1));

	if(acquiredTheLockOnPim)
	{
		// If we are here, pim mutex has been successfully locked - let's store it in a safe RAII lock.
		boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim, boost::adopt_lock);

		if(nullptr != curInt)
			curInt->update();

		if(settings["video"]["showfps"].Bool())
			drawFPSCounter();

		SDL_UpdateTexture(screenTexture, nullptr, screen->pixels, screen->pitch);

		SDL_RenderClear(mainRenderer);
		SDL_RenderCopy(mainRenderer, screenTexture, nullptr, nullptr);

		CCS->curh->render();

		SDL_RenderPresent(mainRenderer);

		windows().onFrameRendered();
	}

	framerateManager().framerateDelay(); // holds a constant FPS
}

CGuiHandler::CGuiHandler()
	: defActionsDef(0)
	, captureChildren(false)
	, multifinger(false)
	, mouseButtonsMask(0)
	, curInt(nullptr)
	, fakeStatusBar(std::make_shared<EmptyStatusBar>())
	, terminate_cond (new CondSh<bool>(false))
{
}

CGuiHandler::~CGuiHandler()
{
	delete terminate_cond;
}

ShortcutHandler & CGuiHandler::shortcutsHandler()
{
	assert(shortcutsHandlerInstance);
	return *shortcutsHandlerInstance;
}

FramerateManager & CGuiHandler::framerateManager()
{
	assert(framerateManagerInstance);
	return *framerateManagerInstance;
}

void CGuiHandler::moveCursorToPosition(const Point & position)
{
	SDL_WarpMouseInWindow(mainWindow, position.x, position.y);
}

bool CGuiHandler::isKeyboardCtrlDown() const
{
#ifdef VCMI_MAC
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LGUI] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RGUI];
#else
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RCTRL];
#endif
}

bool CGuiHandler::isKeyboardAltDown() const
{
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LALT] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RALT];
}

bool CGuiHandler::isKeyboardShiftDown() const
{
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RSHIFT];
}

void CGuiHandler::breakEventHandling()
{
	eventDispatcher().allowEventHandling(false);
}

const Point & CGuiHandler::getCursorPosition() const
{
	return cursorPosition;
}

Point CGuiHandler::screenDimensions() const
{
	return Point(screen->w, screen->h);
}

bool CGuiHandler::isMouseButtonPressed() const
{
	return mouseButtonsMask > 0;
}

bool CGuiHandler::isMouseButtonPressed(MouseButton button) const
{
	static_assert(static_cast<uint32_t>(MouseButton::LEFT)   == SDL_BUTTON_LEFT,   "mismatch between VCMI and SDL enum!");
	static_assert(static_cast<uint32_t>(MouseButton::MIDDLE) == SDL_BUTTON_MIDDLE, "mismatch between VCMI and SDL enum!");
	static_assert(static_cast<uint32_t>(MouseButton::RIGHT)  == SDL_BUTTON_RIGHT,  "mismatch between VCMI and SDL enum!");
	static_assert(static_cast<uint32_t>(MouseButton::EXTRA1) == SDL_BUTTON_X1,     "mismatch between VCMI and SDL enum!");
	static_assert(static_cast<uint32_t>(MouseButton::EXTRA2) == SDL_BUTTON_X2,     "mismatch between VCMI and SDL enum!");

	uint32_t index = static_cast<uint32_t>(button);
	return mouseButtonsMask & SDL_BUTTON(index);
}

void CGuiHandler::drawFPSCounter()
{
	static SDL_Rect overlay = { 0, 0, 64, 32};
	uint32_t black = SDL_MapRGB(screen->format, 10, 10, 10);
	SDL_FillRect(screen, &overlay, black);
	std::string fps = std::to_string(framerateManager().getFramerate());
	graphics->fonts[FONT_BIG]->renderTextLeft(screen, fps, Colors::YELLOW, Point(10, 10));
}

bool CGuiHandler::amIGuiThread()
{
	return inGuiThread.get() && *inGuiThread;
}

void CGuiHandler::pushUserEvent(EUserEvent usercode)
{
	pushUserEvent(usercode, nullptr);
}

void CGuiHandler::pushUserEvent(EUserEvent usercode, void * userdata)
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	event.user.code = static_cast<int32_t>(usercode);
	event.user.data1 = userdata;
	SDL_PushEvent(&event);
}

IScreenHandler & CGuiHandler::screenHandler()
{
	return *screenHandlerInstance;
}

EventDispatcher & CGuiHandler::eventDispatcher()
{
	return *eventDispatcherInstance;
}

WindowHandler & CGuiHandler::windows()
{
	assert(windowHandlerInstance);
	return *windowHandlerInstance;
}

std::shared_ptr<IStatusBar> CGuiHandler::statusbar()
{
	auto locked = currentStatusBar.lock();

	if (!locked)
		return fakeStatusBar;

	return locked;
}

void CGuiHandler::setStatusbar(std::shared_ptr<IStatusBar> newStatusBar)
{
	currentStatusBar = newStatusBar;
}

void CGuiHandler::onScreenResize()
{
	screenHandler().onScreenResize();
	windows().onScreenResize();
}
