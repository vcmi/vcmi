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

#include "CIntObject.h"
#include "CursorHandler.h"
#include "ShortcutHandler.h"
#include "FramerateManager.h"
#include "WindowHandler.h"
#include "EventDispatcher.h"
#include "../eventsSDL/InputHandler.h"

#include "../CGameInfo.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../render/Canvas.h"
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

ObjectConstruction::ObjectConstruction(CIntObject *obj)
{
	GH.createdObj.push_front(obj);
	GH.captureChildren = true;
}

ObjectConstruction::~ObjectConstruction()
{
	assert(!GH.createdObj.empty());
	GH.createdObj.pop_front();
	GH.captureChildren = !GH.createdObj.empty();
}

void CGuiHandler::init()
{
	inGuiThread = true;

	eventDispatcherInstance = std::make_unique<EventDispatcher>();
	windowHandlerInstance = std::make_unique<WindowHandler>();
	screenHandlerInstance = std::make_unique<ScreenHandler>();
	renderHandlerInstance = std::make_unique<RenderHandler>();
	shortcutsHandlerInstance = std::make_unique<ShortcutHandler>();
	inputHandlerInstance = std::make_unique<InputHandler>(); // Must be after windowHandlerInstance and shortcutsHandlerInstance
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

		screenHandlerInstance->updateScreenTexture();

		windows().onFrameRendered();
		CCS->curh->update();
	}

	screenHandlerInstance->presetScreenTexture();
	framerate().framerateDelay(); // holds a constant FPS
}

CGuiHandler::CGuiHandler()
	: captureChildren(false)
	, curInt(nullptr)
	, fakeStatusBar(std::make_shared<EmptyStatusBar>())
{
}

CGuiHandler::~CGuiHandler()
{
	// enforce deletion order on shutdown
	// all UI elements including adventure map must be destroyed before Gui Handler
	// proper solution would be removal of adventureInt global
	adventureInt.reset();
}

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

bool CGuiHandler::isKeyboardCmdDown() const
{
	return inputHandlerInstance->isKeyboardCmdDown();
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
	return screenHandlerInstance->getLogicalResolution();
}

void CGuiHandler::drawFPSCounter()
{
	Canvas target = GH.screenHandler().getScreenCanvas();
	Rect targetArea(0, screenDimensions().y - 20, 48, 11);
	std::string fps = std::to_string(framerate().getFramerate())+" FPS";

	target.drawColor(targetArea, ColorRGBA(10, 10, 10));
	target.drawText(targetArea.center(), EFonts::FONT_SMALL, Colors::WHITE, ETextAlignment::CENTER, fps);
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
		screenHandler().onScreenResize();

	windows().onScreenResize();
	CCS->curh->onScreenResize();
}
