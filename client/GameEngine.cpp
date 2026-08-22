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
#include "Profiler.h"
#include "GameEngine.h"
#include "GameLibrary.h"
#include "Discord.h"

#include "gui/CIntObject.h"
#include "gui/CursorHandler.h"
#include "gui/ShortcutHandler.h"
#include "gui/FramerateManager.h"
#include "gui/WindowHandler.h"
#include "gui/EventDispatcher.h"
#include "events/InputHandler.h"

#include "media/CMusicHandler.h"
#include "media/CSoundHandler.h"
#include "media/CVideoHandler.h"
#include "media/CEmptyVideoPlayer.h"

#include "adventureMap/AdventureMapInterface.h"
#include "render/Canvas.h"
#include "render/Colors.h"
#include "render/IFont.h"
#include "render/EFont.h"
#include "render/ScreenHandler.h"
#include "render/RenderHandler.h"
#include "GameEngineUser.h"
#include "battle/BattleInterface.h"

#include "../lib/AsyncRunner.h"
#include "../lib/CConfigHandler.h"
#include "../lib/texts/TextOperations.h"
#include "../lib/texts/CGeneralTextHandler.h"

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

	discordInstance = std::make_unique<Discord>();

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
	VCMI_PROFILE_N("Engine: handle events");
	events().dispatchTimer(framerate().consumeElapsedMilliseconds());

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
	VCMI_PROFILE_THREAD("GUI");
	VCMI_PROFILE_PLOT_LINE("Engine: frames per second");
	VCMI_PROFILE_PLOT_LINE("Engine: frame time (ms)");

	for (;;)
	{
		VCMI_PROFILE_N("Engine: main loop iteration");
		input().fetchEvents();

		// A frame with nothing new to show would still hold interfaceMutex, which is the lock the
		// network thread needs to produce the next one. The time bound is only a safety net.
		const auto now = std::chrono::steady_clock::now();

		if(!engineUser->wantsFrameRendered() && now - lastFrameRendered < maxFrameSkipDuration)
			continue; // this frame does not happen at all, its content comes with the next one

		lastFrameRendered = now;
		updateFrame();
		screenHandlerInstance->presentScreenTexture();

		{
			VCMI_PROFILE_N("Engine: framerate delay");
			framerate().framerateDelay(); // holds a constant FPS
		}

		VCMI_PROFILE_PLOT_FLOAT("Engine: frames per second", framerate().getFramerate());
		VCMI_PROFILE_PLOT_FLOAT("Engine: frame time (ms)", framerate().getElapsedMilliseconds());
		VCMI_PROFILE_PLOT("Engine: open windows", windows().count());
	}
}

void GameEngine::updateFrame()
{
	VCMI_PROFILE_N("Engine: update frame");
	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	engineUser->onUpdate();

	handleEvents();

	// before the redraw, so that a window that was only covered can reclaim its layer
	screenHandlerInstance->clearReleasedLayers();

	windows().simpleRedraw();

	if (settings["video"]["performanceOverlay"]["show"].Bool())
		drawPerformanceOverlay();

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

bool GameEngine::isRoeData() const
{
	return LIBRARY->isRoeData();
}

bool GameEngine::isDemoData() const
{
	return LIBRARY->isDemoData();
}

void GameEngine::drawPerformanceOverlay()
{
	VCMI_PROFILE_N("Engine: performance overlay");
	auto font = EFonts::FONT_SMALL;
	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);

	Canvas target = screenHandler().getScreenCanvas();

	auto powerState = ENGINE->input().getPowerState();
	std::string powerSymbol = ""; // add symbol if emoji are supported (e.g. VCMI extras)
	if(powerState.powerState == PowerStateMode::ON_BATTERY)
		powerSymbol = fontPtr->canRepresentCharacter("🔋") ? "🔋 " : (LIBRARY->generaltexth->translate("vcmi.overlay.battery") + " ");
	else if(powerState.powerState == PowerStateMode::CHARGING)
		powerSymbol = fontPtr->canRepresentCharacter("🔌") ? "🔌 " : (LIBRARY->generaltexth->translate("vcmi.overlay.charging") + " ");

	std::string fps = std::to_string(framerate().getFramerate())+" FPS";
	std::string time = TextOperations::getFormattedTimeLocal(std::time(nullptr));
	std::string power = powerState.powerState == PowerStateMode::UNKNOWN ? "" : powerSymbol + std::to_string(powerState.percent) + "%";

	std::string textToDisplay = time + (power.empty() ? "" : " | " + power) + " | " + fps;

	maxPerformanceOverlayTextWidth = std::max(maxPerformanceOverlayTextWidth, static_cast<int>(fontPtr->getStringWidth(textToDisplay))); // do not get smaller (can cause graphical glitches)

	Rect targetArea;
	std::string edge = settings["video"]["performanceOverlay"]["edge"].String();
	int marginTopBottom = settings["video"]["performanceOverlay"]["marginTopBottom"].Integer();
	int marginLeftRight = settings["video"]["performanceOverlay"]["marginLeftRight"].Integer();

	Point boxSize(maxPerformanceOverlayTextWidth + 6, fontPtr->getLineHeight() + 2);

	if (edge == "topleft")
		targetArea = Rect(marginLeftRight, marginTopBottom, boxSize.x, boxSize.y);
	else if (edge == "topright")
		targetArea = Rect(screenDimensions().x - marginLeftRight - boxSize.x, marginTopBottom, boxSize.x, boxSize.y);
	else if (edge == "bottomleft")
		targetArea = Rect(marginLeftRight, screenDimensions().y - marginTopBottom - boxSize.y, boxSize.x, boxSize.y);
	else if (edge == "bottomright")
		targetArea = Rect(screenDimensions().x - marginLeftRight - boxSize.x, screenDimensions().y - marginTopBottom - boxSize.y, boxSize.x, boxSize.y);

	target.drawColor(targetArea.resize(1), Colors::BRIGHT_YELLOW);
	target.drawColor(targetArea, ColorRGBA(0, 0, 0));
	target.drawText(targetArea.center(), font, Colors::WHITE, ETextAlignment::CENTER, textToDisplay);
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

void GameEngine::onScreenResize(bool resolutionChanged, bool windowResized)
{
	VCMI_PROFILE_N("Engine: screen resize");
	if(resolutionChanged)
		if(!screenHandler().onScreenResize(windowResized))
			return;

	windows().onScreenResize();
	ENGINE->cursor().onScreenResize();
}

void GameEngine::setEngineUser(IGameEngineUser * user)
{
	engineUser = user;
}
