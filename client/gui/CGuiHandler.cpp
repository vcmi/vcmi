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
#include "../eventsSDL/InputHandler.h"

#include "../CGameInfo.h"
#include "../render/Colors.h"
#include "../render/Graphics.h"
#include "../render/IFont.h"
#include "../render/EFont.h"
#include "../renderSDL/ScreenHandler.h"
#include "../renderSDL/RenderHandler.h"
#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../battle/BattleInterface.h"

#include "../../lib/CThreadHelper.h"
#include "../../lib/CConfigHandler.h"

#include <SDL_render.h>

CGuiHandler GH;

static thread_local bool inGuiThread = false;

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
	inGuiThread = true;

	inputHandlerInstance = std::make_unique<InputHandler>();
	eventDispatcherInstance = std::make_unique<EventDispatcher>();
	windowHandlerInstance = std::make_unique<WindowHandler>();
	screenHandlerInstance = std::make_unique<ScreenHandler>();
	renderHandlerInstance = std::make_unique<RenderHandler>();
	shortcutsHandlerInstance = std::make_unique<ShortcutHandler>();
	framerateManagerInstance = std::make_unique<FramerateManager>(settings["video"]["targetfps"].Integer());
}

void CGuiHandler::handleEvents()
{
	events().dispatchTimer(framerate().getElapsedMilliseconds());

	//player interface may want special event handling
	if(nullptr != LOCPLINT && LOCPLINT->capturedAllEvents())
		return;

	input().processEvents();
}

void CGuiHandler::fakeMouseMove()
{
	dispatchMainThread([](){
		GH.events().dispatchMouseMoved(Point(0, 0), GH.getCursorPosition());
	});
}

void CGuiHandler::startTextInput(const Rect & whereInput)
{
	input().startTextInput(whereInput);
}

void CGuiHandler::stopTextInput()
{
	input().stopTextInput();
}

void CGuiHandler::renderFrame()
{
	{
		boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);

		if (nullptr != curInt)
			curInt->update();

		if (settings["video"]["showfps"].Bool())
			drawFPSCounter();
	}

	SDL_UpdateTexture(screenTexture, nullptr, screen->pixels, screen->pitch);

	SDL_RenderClear(mainRenderer);
	SDL_RenderCopy(mainRenderer, screenTexture, nullptr, nullptr);

	{
		boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);

		CCS->curh->render();

		windows().onFrameRendered();
	}

	SDL_RenderPresent(mainRenderer);
	framerate().framerateDelay(); // holds a constant FPS
}

CGuiHandler::CGuiHandler()
	: defActionsDef(0)
	, captureChildren(false)
	, curInt(nullptr)
	, fakeStatusBar(std::make_shared<EmptyStatusBar>())
{
}

CGuiHandler::~CGuiHandler() = default;

ShortcutHandler & CGuiHandler::shortcuts()
{
	assert(shortcutsHandlerInstance);
	return *shortcutsHandlerInstance;
}

FramerateManager & CGuiHandler::framerate()
{
	assert(framerateManagerInstance);
	return *framerateManagerInstance;
}

bool CGuiHandler::isKeyboardCtrlDown() const
{
	return inputHandlerInstance->isKeyboardCtrlDown();
}

bool CGuiHandler::isKeyboardAltDown() const
{
	return inputHandlerInstance->isKeyboardAltDown();
}

bool CGuiHandler::isKeyboardShiftDown() const
{
	return inputHandlerInstance->isKeyboardShiftDown();
}

const Point & CGuiHandler::getCursorPosition() const
{
	return inputHandlerInstance->getCursorPosition();
}

Point CGuiHandler::screenDimensions() const
{
	return Point(screen->w, screen->h);
}

void CGuiHandler::drawFPSCounter()
{
	int x = 7;
	int y = screen->h-20;
	int width3digitFPSIncludingPadding = 48;
	int heightFPSTextIncludingPadding = 11;
	SDL_Rect overlay = { x, y, width3digitFPSIncludingPadding, heightFPSTextIncludingPadding};
	uint32_t black = SDL_MapRGB(screen->format, 10, 10, 10);
	SDL_FillRect(screen, &overlay, black);

	std::string fps = std::to_string(framerate().getFramerate())+" FPS";

	graphics->fonts[FONT_SMALL]->renderTextLeft(screen, fps, Colors::WHITE, Point(8, screen->h-22));
}

bool CGuiHandler::amIGuiThread()
{
	return inGuiThread;
}

void CGuiHandler::dispatchMainThread(const std::function<void()> & functor)
{
	inputHandlerInstance->dispatchMainThread(functor);
}

IScreenHandler & CGuiHandler::screenHandler()
{
	return *screenHandlerInstance;
}

IRenderHandler & CGuiHandler::renderHandler()
{
	return *renderHandlerInstance;
}

EventDispatcher & CGuiHandler::events()
{
	return *eventDispatcherInstance;
}

InputHandler & CGuiHandler::input()
{
	return *inputHandlerInstance;
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

void CGuiHandler::onScreenResize(bool resolutionChanged)
{
	if(resolutionChanged)
	{
		screenHandler().onScreenResize();
	}
	windows().onScreenResize();
}
