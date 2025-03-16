/*
 * GameEngine.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GameEngine.h"

#include "gui/CIntObject.h"
#include "gui/CursorHandler.h"
#include "gui/ShortcutHandler.h"
#include "gui/FramerateManager.h"
#include "gui/WindowHandler.h"
#include "gui/EventDispatcher.h"
#include "eventsSDL/InputHandler.h"

#include "media/CMusicHandler.h"
#include "media/CSoundHandler.h"
#include "media/CVideoHandler.h"
#include "media/CEmptyVideoPlayer.h"

#include "adventureMap/AdventureMapInterface.h"
#include "render/Canvas.h"
#include "render/Colors.h"
#include "render/IFont.h"
#include "render/EFont.h"
#include "renderSDL/ScreenHandler.h"
#include "renderSDL/RenderHandler.h"
#include "GameEngineUser.h"
#include "battle/BattleInterface.h"

#include "../lib/AsyncRunner.h"
#include "../lib/CConfigHandler.h"

#include <SDL_render.h>

std::unique_ptr<GameEngine> ENGINE;

static thread_local bool inGuiThread = false;

ObjectConstruction::ObjectConstruction(CIntObject *obj)
{
	ENGINE->createdObj.push_front(obj);
	ENGINE->captureChildren = true;
}

ObjectConstruction::~ObjectConstruction()
{
	assert(!ENGINE->createdObj.empty());
	ENGINE->createdObj.pop_front();
	ENGINE->captureChildren = !ENGINE->createdObj.empty();
}

GameEngine::GameEngine()
	: captureChildren(false)
	, fakeStatusBar(std::make_shared<EmptyStatusBar>())
{
	inGuiThread = true;

	eventDispatcherInstance = std::make_unique<EventDispatcher>();
	windowHandlerInstance = std::make_unique<WindowHandler>();
	screenHandlerInstance = std::make_unique<ScreenHandler>();
	renderHandlerInstance = std::make_unique<RenderHandler>();
	shortcutsHandlerInstance = std::make_unique<ShortcutHandler>();
	inputHandlerInstance = std::make_unique<InputHandler>(); // Must be after windowHandlerInstance and shortcutsHandlerInstance
	framerateManagerInstance = std::make_unique<FramerateManager>(settings["video"]["targetfps"].Integer());

#ifndef ENABLE_VIDEO
	videoPlayerInstance = std::make_unique<CEmptyVideoPlayer>();
#else
	if (settings["session"]["disableVideo"].Bool())
		videoPlayerInstance = std::make_unique<CEmptyVideoPlayer>();
	else
		videoPlayerInstance = std::make_unique<CVideoPlayer>();
#endif

	soundPlayerInstance = std::make_unique<CSoundHandler>();
	musicPlayerInstance = std::make_unique<CMusicHandler>();
	sound().setVolume(settings["general"]["sound"].Integer());
	music().setVolume(settings["general"]["music"].Integer());
	cursorHandlerInstance = std::make_unique<CursorHandler>();

	asyncTasks = std::make_unique<AsyncRunner>();
}

void GameEngine::handleEvents()
{
	events().dispatchTimer(framerate().getElapsedMilliseconds());

	//player interface may want special event handling
	if(engineUser->capturedAllEvents())
		return;

	input().processEvents();
}

void GameEngine::fakeMouseMove()
{
	dispatchMainThread([](){
		ENGINE->events().dispatchMouseMoved(Point(0, 0), ENGINE->getCursorPosition());
	});
}

[[noreturn]] void GameEngine::mainLoop()
{
	for (;;)
	{
		input().fetchEvents();
		updateFrame();
		screenHandlerInstance->presentScreenTexture();
		framerate().framerateDelay(); // holds a constant FPS
	}
}

void GameEngine::updateFrame()
{
	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	engineUser->onUpdate();

	handleEvents();
	windows().simpleRedraw();

	if (settings["video"]["showfps"].Bool())
		drawFPSCounter();

	screenHandlerInstance->updateScreenTexture();

	windows().onFrameRendered();
	ENGINE->cursor().update();
}

GameEngine::~GameEngine()
{
	// enforce deletion order on shutdown
	// all UI elements including adventure map must be destroyed before Gui Handler
	// proper solution would be removal of adventureInt global
	adventureInt.reset();
}

ShortcutHandler & GameEngine::shortcuts()
{
	assert(shortcutsHandlerInstance);
	return *shortcutsHandlerInstance;
}

FramerateManager & GameEngine::framerate()
{
	assert(framerateManagerInstance);
	return *framerateManagerInstance;
}

bool GameEngine::isKeyboardCtrlDown() const
{
	return inputHandlerInstance->isKeyboardCtrlDown();
}

bool GameEngine::isKeyboardCmdDown() const
{
	return inputHandlerInstance->isKeyboardCmdDown();
}

bool GameEngine::isKeyboardAltDown() const
{
	return inputHandlerInstance->isKeyboardAltDown();
}

bool GameEngine::isKeyboardShiftDown() const
{
	return inputHandlerInstance->isKeyboardShiftDown();
}

const Point & GameEngine::getCursorPosition() const
{
	return inputHandlerInstance->getCursorPosition();
}

Point GameEngine::screenDimensions() const
{
	return screenHandlerInstance->getLogicalResolution();
}

void GameEngine::drawFPSCounter()
{
	Canvas target = screenHandler().getScreenCanvas();
	Rect targetArea(0, screenDimensions().y - 20, 48, 11);
	std::string fps = std::to_string(framerate().getFramerate())+" FPS";

	target.drawColor(targetArea, ColorRGBA(10, 10, 10));
	target.drawText(targetArea.center(), EFonts::FONT_SMALL, Colors::WHITE, ETextAlignment::CENTER, fps);
}

bool GameEngine::amIGuiThread()
{
	return inGuiThread;
}

void GameEngine::dispatchMainThread(const std::function<void()> & functor)
{
	inputHandlerInstance->dispatchMainThread(functor);
}

IScreenHandler & GameEngine::screenHandler()
{
	return *screenHandlerInstance;
}

IRenderHandler & GameEngine::renderHandler()
{
	return *renderHandlerInstance;
}

EventDispatcher & GameEngine::events()
{
	return *eventDispatcherInstance;
}

InputHandler & GameEngine::input()
{
	return *inputHandlerInstance;
}

WindowHandler & GameEngine::windows()
{
	assert(windowHandlerInstance);
	return *windowHandlerInstance;
}

std::shared_ptr<IStatusBar> GameEngine::statusbar()
{
	auto locked = currentStatusBar.lock();

	if (!locked)
		return fakeStatusBar;

	return locked;
}

void GameEngine::setStatusbar(const std::shared_ptr<IStatusBar> & newStatusBar)
{
	currentStatusBar = newStatusBar;
}

void GameEngine::onScreenResize(bool resolutionChanged)
{
	if(resolutionChanged)
		screenHandler().onScreenResize();

	windows().onScreenResize();
	ENGINE->cursor().onScreenResize();
}

void GameEngine::setEngineUser(IGameEngineUser * user)
{
	engineUser = user;
}
