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
		assert(CPlayerInterface::pim);
		boost::unique_lock lock(*CPlayerInterface::pim);
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
		boost::recursive_mutex::scoped_lock un(*CPlayerInterface::pim);

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
	static SDL_Rect overlay = { 0, 0, 64, 32};
	uint32_t black = SDL_MapRGB(screen->format, 10, 10, 10);
	SDL_FillRect(screen, &overlay, black);
	std::string fps = std::to_string(framerate().getFramerate());
	graphics->fonts[FONT_BIG]->renderTextLeft(screen, fps, Colors::YELLOW, Point(10, 10));
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

void CGuiHandler::onScreenResize()
{
	screenHandler().onScreenResize();
	windows().onScreenResize();
}
