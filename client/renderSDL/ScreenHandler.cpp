/*
 * ScreenHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ScreenHandler.h"

#include "../../lib/CConfigHandler.h"
#include "../gui/CGuiHandler.h"
#include "../eventsSDL/NotificationHandler.h"
#include "../gui/WindowHandler.h"
#include "CMT.h"
#include "SDL_Extensions.h"

#ifdef VCMI_ANDROID
#include "../lib/CAndroidVMHelper.h"
#endif

#ifdef VCMI_IOS
#	include "ios/utils.h"
#endif

#include <SDL.h>

// TODO: should be made into a private members of ScreenHandler
static SDL_Window * mainWindow = nullptr;
SDL_Renderer * mainRenderer = nullptr;
SDL_Texture * screenTexture = nullptr;
SDL_Surface * screen = nullptr; //main screen surface
SDL_Surface * screen2 = nullptr; //and hlp surface (used to store not-active interfaces layer)
SDL_Surface * screenBuf = screen; //points to screen (if only advmapint is present) or screen2 (else) - should be used when updating controls which are not regularly redrawed

static const std::string NAME_AFFIX = "client";
static const std::string NAME = GameConstants::VCMI_VERSION + std::string(" (") + NAME_AFFIX + ')'; //application name

std::tuple<int, int> ScreenHandler::getSupportedScalingRange() const
{
	// H3 resolution, any resolution smaller than that is not correctly supported
	static const Point minResolution = {800, 600};
	// arbitrary limit on *downscaling*. Allow some downscaling, if requested by user. Should be generally limited to 100+ for all but few devices
	static const double minimalScaling = 50;

	Point renderResolution = getRenderResolution();
	double reservedAreaWidth = settings["video"]["reservedWidth"].Float();
	Point availableResolution = Point(renderResolution.x * (1 - reservedAreaWidth), renderResolution.y);

	double maximalScalingWidth = 100.0 * availableResolution.x / minResolution.x;
	double maximalScalingHeight = 100.0 * availableResolution.y / minResolution.y;
	double maximalScaling = std::min(maximalScalingWidth, maximalScalingHeight);

	return { minimalScaling, maximalScaling };
}

Rect ScreenHandler::convertLogicalPointsToWindow(const Rect & input) const
{
	Rect result;

	// FIXME: use SDL_RenderLogicalToWindow instead? Needs to be tested on ios

	float scaleX, scaleY;
	SDL_Rect viewport;
	SDL_RenderGetScale(mainRenderer, &scaleX, &scaleY);
	SDL_RenderGetViewport(mainRenderer, &viewport);

#ifdef VCMI_IOS
	// TODO ios: looks like SDL bug actually, try fixing there
	const auto nativeScale = iOS_utils::screenScale();
	scaleX /= nativeScale;
	scaleY /= nativeScale;
#endif

	result.x = (viewport.x + input.x) * scaleX;
	result.y = (viewport.y + input.y) * scaleY;
	result.w = input.w * scaleX;
	result.h = input.h * scaleY;

	return result;
}

Point ScreenHandler::getPreferredLogicalResolution() const
{
	Point renderResolution = getRenderResolution();
	double reservedAreaWidth = settings["video"]["reservedWidth"].Float();
	Point availableResolution = Point(renderResolution.x * (1 - reservedAreaWidth), renderResolution.y);

	auto [minimalScaling, maximalScaling] = getSupportedScalingRange();

	int userScaling = settings["video"]["resolution"]["scaling"].Integer();
	int scaling = std::clamp(userScaling, minimalScaling, maximalScaling);

	Point logicalResolution = availableResolution * 100.0 / scaling;

	return logicalResolution;
}

Point ScreenHandler::getRenderResolution() const
{
	assert(mainRenderer != nullptr);

	Point result;
	SDL_GetRendererOutputSize(mainRenderer, &result.x, &result.y);

	return result;
}

Point ScreenHandler::getPreferredWindowResolution() const
{
	if (getPreferredWindowMode() == EWindowMode::FULLSCREEN_BORDERLESS_WINDOWED)
	{
		SDL_Rect bounds;
		if (SDL_GetDisplayBounds(getPreferredDisplayIndex(), &bounds) == 0)
			return Point(bounds.w, bounds.h);
	}

	const JsonNode & video = settings["video"];
	int width = video["resolution"]["width"].Integer();
	int height = video["resolution"]["height"].Integer();

	return Point(width, height);
}

int ScreenHandler::getPreferredDisplayIndex() const
{
#ifdef VCMI_MOBILE
	// Assuming no multiple screens on Android / ios?
	return 0;
#else
	if (mainWindow != nullptr)
	{
		int result = SDL_GetWindowDisplayIndex(mainWindow);
		if (result >= 0)
			return result;
	}

	return settings["video"]["displayIndex"].Integer();
#endif
}

EWindowMode ScreenHandler::getPreferredWindowMode() const
{
#ifdef VCMI_MOBILE
	// On Android / ios game will always render to screen size
	return EWindowMode::FULLSCREEN_BORDERLESS_WINDOWED;
#else
	const JsonNode & video = settings["video"];
	bool fullscreen = video["fullscreen"].Bool();
	bool realFullscreen = settings["video"]["realFullscreen"].Bool();

	if (!fullscreen)
		return EWindowMode::WINDOWED;

	if (realFullscreen)
		return EWindowMode::FULLSCREEN_EXCLUSIVE;
	else
		return EWindowMode::FULLSCREEN_BORDERLESS_WINDOWED;
#endif
}

ScreenHandler::ScreenHandler()
{
#ifdef VCMI_WINDOWS
	// set VCMI as "per-monitor DPI awareness". This completely disables any DPI-scaling by system.
	// Might not be the best solution since VCMI can't automatically adjust to DPI changes (including moving to monitors with different DPI scaling)
	// However this fixed unintuitive bug where player selects specific resolution for windowed mode, but ends up with completely different one due to scaling
	// NOTE: requires SDL 2.24.
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitor");
#endif

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO))
	{
		logGlobal->error("Something was wrong: %s", SDL_GetError());
		exit(-1);
	}

	const auto & logCallback = [](void * userdata, int category, SDL_LogPriority priority, const char * message)
	{
		logGlobal->debug("SDL(category %d; priority %d) %s", category, priority, message);
	};

	SDL_LogSetOutputFunction(logCallback, nullptr);

#ifdef VCMI_ANDROID
	// manually setting egl pixel format, as a possible solution for sdl2<->android problem
	// https://bugzilla.libsdl.org/show_bug.cgi?id=2291
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
#endif // VCMI_ANDROID

	validateSettings();
	recreateWindowAndScreenBuffers();
}

void ScreenHandler::recreateWindowAndScreenBuffers()
{
	destroyScreenBuffers();

	if(mainWindow == nullptr)
		initializeWindow();
	else
		updateWindowState();

	initializeScreenBuffers();

	if(!settings["session"]["headless"].Bool() && settings["general"]["notifications"].Bool())
	{
		NotificationHandler::init(mainWindow);
	}
}

void ScreenHandler::updateWindowState()
{
#ifndef VCMI_MOBILE
	int displayIndex = getPreferredDisplayIndex();

	switch(getPreferredWindowMode())
	{
		case EWindowMode::FULLSCREEN_EXCLUSIVE:
		{
			// for some reason, VCMI fails to switch from FULLSCREEN_BORDERLESS_WINDOWED to FULLSCREEN_EXCLUSIVE directly
			// Switch to windowed mode first to avoid this bug
			SDL_SetWindowFullscreen(mainWindow, 0);
			SDL_SetWindowFullscreen(mainWindow, SDL_WINDOW_FULLSCREEN);

			SDL_DisplayMode mode;
			SDL_GetDesktopDisplayMode(displayIndex, &mode);
			Point resolution = getPreferredWindowResolution();

			mode.w = resolution.x;
			mode.h = resolution.y;

			SDL_SetWindowDisplayMode(mainWindow, &mode);
			SDL_SetWindowPosition(mainWindow, SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex));

			return;
		}
		case EWindowMode::FULLSCREEN_BORDERLESS_WINDOWED:
		{
			SDL_SetWindowFullscreen(mainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
			SDL_SetWindowPosition(mainWindow, SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex));
			return;
		}
		case EWindowMode::WINDOWED:
		{
			Point resolution = getPreferredWindowResolution();
			SDL_SetWindowFullscreen(mainWindow, 0);
			SDL_SetWindowSize(mainWindow, resolution.x, resolution.y);
			SDL_SetWindowPosition(mainWindow, SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex));
			return;
		}
	}
#endif
}

void ScreenHandler::initializeWindow()
{
	mainWindow = createWindow();

	if(mainWindow == nullptr)
	{
		const char * error = SDL_GetError();
		Point dimensions = getPreferredWindowResolution();

		std::string messagePattern = "Failed to create SDL Window of size %d x %d. Reason: %s";
		std::string message = boost::str(boost::format(messagePattern) % dimensions.x % dimensions.y % error);

		handleFatalError(message, true);
	}

	//create first available renderer if preferred not set. Use no flags, so HW accelerated will be preferred but SW renderer also will possible
	mainRenderer = SDL_CreateRenderer(mainWindow, getPreferredRenderingDriver(), 0);

	if(mainRenderer == nullptr)
		throw std::runtime_error("Unable to create renderer\n");

	SDL_RendererInfo info;
	SDL_GetRendererInfo(mainRenderer, &info);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
	logGlobal->info("Created renderer %s", info.name);
}

void ScreenHandler::initializeScreenBuffers()
{
#ifdef VCMI_ENDIAN_BIG
	int bmask = 0xff000000;
	int gmask = 0x00ff0000;
	int rmask = 0x0000ff00;
	int amask = 0x000000ff;
#else
	int bmask = 0x000000ff;
	int gmask = 0x0000ff00;
	int rmask = 0x00ff0000;
	int amask = 0xFF000000;
#endif

	auto logicalSize = getPreferredLogicalResolution();
	SDL_RenderSetLogicalSize(mainRenderer, logicalSize.x, logicalSize.y);

	screen = SDL_CreateRGBSurface(0, logicalSize.x, logicalSize.y, 32, rmask, gmask, bmask, amask);
	if(nullptr == screen)
	{
		logGlobal->error("Unable to create surface %dx%d with %d bpp: %s", logicalSize.x, logicalSize.y, 32, SDL_GetError());
		throw std::runtime_error("Unable to create surface");
	}
	//No blending for screen itself. Required for proper cursor rendering.
	SDL_SetSurfaceBlendMode(screen, SDL_BLENDMODE_NONE);

	screenTexture = SDL_CreateTexture(mainRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, logicalSize.x, logicalSize.y);

	if(nullptr == screenTexture)
	{
		logGlobal->error("Unable to create screen texture");
		logGlobal->error(SDL_GetError());
		throw std::runtime_error("Unable to create screen texture");
	}

	screen2 = CSDL_Ext::copySurface(screen);

	if(nullptr == screen2)
	{
		throw std::runtime_error("Unable to copy surface\n");
	}

	if (GH.windows().count() > 1)
		screenBuf = screen2;
	else
		screenBuf = screen;

	clearScreen();
}

SDL_Window * ScreenHandler::createWindowImpl(Point dimensions, int flags, bool center)
{
	int displayIndex = getPreferredDisplayIndex();
	int positionFlags = center ? SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex) : SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex);

	return SDL_CreateWindow(NAME.c_str(), positionFlags, positionFlags, dimensions.x, dimensions.y, flags);
}

SDL_Window * ScreenHandler::createWindow()
{
#ifndef VCMI_MOBILE
	Point dimensions = getPreferredWindowResolution();

	switch(getPreferredWindowMode())
	{
		case EWindowMode::FULLSCREEN_EXCLUSIVE:
			return createWindowImpl(dimensions, SDL_WINDOW_FULLSCREEN, false);

		case EWindowMode::FULLSCREEN_BORDERLESS_WINDOWED:
			return createWindowImpl(Point(), SDL_WINDOW_FULLSCREEN_DESKTOP, false);

		case EWindowMode::WINDOWED:
			return createWindowImpl(dimensions, SDL_WINDOW_RESIZABLE, true);

		default:
			return nullptr;
	};
#endif

#ifdef VCMI_IOS
	SDL_SetHint(SDL_HINT_IOS_HIDE_HOME_INDICATOR, "1");
	SDL_SetHint(SDL_HINT_RETURN_KEY_HIDES_IME, "1");

	uint32_t windowFlags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI;
	SDL_Window * result = createWindowImpl(Point(), windowFlags | SDL_WINDOW_METAL, false);

	if(result != nullptr)
		return result;

	logGlobal->warn("Metal unavailable, using OpenGLES");
	return createWindowImpl(Point(), windowFlags, false);
#endif

#ifdef VCMI_ANDROID
	return createWindowImpl(Point(), SDL_WINDOW_FULLSCREEN, false);
#endif
}

void ScreenHandler::onScreenResize()
{
	recreateWindowAndScreenBuffers();
}

void ScreenHandler::validateSettings()
{
#ifndef VCMI_MOBILE
	{
		int displayIndex = settings["video"]["displayIndex"].Integer();
		int displaysCount = SDL_GetNumVideoDisplays();

		if (displayIndex >= displaysCount)
		{
			Settings writer = settings.write["video"]["displayIndex"];
			writer->Float() = 0;
		}
	}

	if (getPreferredWindowMode() == EWindowMode::WINDOWED)
	{
		//we only check that our desired window size fits on screen
		int displayIndex = getPreferredDisplayIndex();
		Point resolution = getPreferredWindowResolution();

		SDL_DisplayMode mode;

		if (SDL_GetDesktopDisplayMode(displayIndex, &mode) == 0)
		{
			if(resolution.x > mode.w || resolution.y > mode.h)
			{
				Settings writer = settings.write["video"]["resolution"];
				writer["width"].Float() = mode.w;
				writer["height"].Float() = mode.h;
			}
		}
	}

	if (getPreferredWindowMode() == EWindowMode::FULLSCREEN_EXCLUSIVE)
	{
		auto legalOptions = getSupportedResolutions();
		Point selectedResolution = getPreferredWindowResolution();

		if(!vstd::contains(legalOptions, selectedResolution))
		{
			// resolution selected for fullscreen mode is not supported by display
			// try to find current display resolution and use it instead as "reasonable default"
			SDL_DisplayMode mode;

			if (SDL_GetDesktopDisplayMode(getPreferredDisplayIndex(), &mode) == 0)
			{
				Settings writer = settings.write["video"]["resolution"];
				writer["width"].Float() = mode.w;
				writer["height"].Float() = mode.h;
			}
		}
	}
#endif
}

int ScreenHandler::getPreferredRenderingDriver() const
{
	int result = -1;
	const JsonNode & video = settings["video"];

	int driversCount = SDL_GetNumRenderDrivers();
	std::string preferredDriverName = video["driver"].String();

	logGlobal->info("Found %d render drivers", driversCount);

	for(int it = 0; it < driversCount; it++)
	{
		SDL_RendererInfo info;
		if (SDL_GetRenderDriverInfo(it, &info) == 0)
		{
			std::string driverName(info.name);

			if(!preferredDriverName.empty() && driverName == preferredDriverName)
			{
				result = it;
				logGlobal->info("\t%s (active)", driverName);
			}
			else
				logGlobal->info("\t%s", driverName);
		}
		else
			logGlobal->info("\t(error)");
	}
	return result;
}

void ScreenHandler::destroyScreenBuffers()
{
	// screenBuf is not a separate surface, but points to either screen or screen2 - just set to null
	screenBuf = nullptr;

	if(nullptr != screen2)
	{
		SDL_FreeSurface(screen2);
		screen2 = nullptr;
	}

	if(nullptr != screen)
	{
		SDL_FreeSurface(screen);
		screen = nullptr;
	}

	if(nullptr != screenTexture)
	{
		SDL_DestroyTexture(screenTexture);
		screenTexture = nullptr;
	}
}

void ScreenHandler::destroyWindow()
{
	if(nullptr != mainRenderer)
	{
		SDL_DestroyRenderer(mainRenderer);
		mainRenderer = nullptr;
	}

	if(nullptr != mainWindow)
	{
		SDL_DestroyWindow(mainWindow);
		mainWindow = nullptr;
	}
}

void ScreenHandler::close()
{
	if(settings["general"]["notifications"].Bool())
		NotificationHandler::destroy();

	destroyScreenBuffers();
	destroyWindow();
	SDL_Quit();
}

void ScreenHandler::clearScreen()
{
	SDL_SetRenderDrawColor(mainRenderer, 0, 0, 0, 255);
	SDL_RenderClear(mainRenderer);
	SDL_RenderPresent(mainRenderer);
}

std::vector<Point> ScreenHandler::getSupportedResolutions() const
{
	int displayID = getPreferredDisplayIndex();
	return getSupportedResolutions(displayID);
}

std::vector<Point> ScreenHandler::getSupportedResolutions( int displayIndex) const
{
	//NOTE: this method is never called on Android/iOS, only on desktop systems

	std::vector<Point> result;

	int modesCount = SDL_GetNumDisplayModes(displayIndex);

	for (int i =0; i < modesCount; ++i)
	{
		SDL_DisplayMode mode;
		if (SDL_GetDisplayMode(displayIndex, i, &mode) == 0)
		{
			Point resolution(mode.w, mode.h);
			result.push_back(resolution);
		}
	}

	boost::range::sort(result, [](const auto & left, const auto & right)
	{
		return left.x * left.y < right.x * right.y;
	});

	// erase potential duplicates, e.g. resolutions with different framerate / bits per pixel
	result.erase(boost::unique(result).end(), result.end());

	return result;
}
