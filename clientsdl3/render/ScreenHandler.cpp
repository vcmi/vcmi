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
#include "GpuResources.h"

#include "SDL_Extensions.h"

#include "CMT.h"
#include "../events/NotificationHandler.h"
#include "GameEngine.h"
#include "GameInstance.h"
#include "CServerHandler.h"
#include "GameChatHandler.h"
#include "gui/CursorHandler.h"
#include "gui/WindowHandler.h"
#include "Canvas.h"
#include "SDLImage.h"

#include "lib/CConfigHandler.h"
#include "lib/constants/StringConstants.h"
#include "lib/VCMIDirs.h"
#include "lib/texts/MetaString.h"

#include <vstd/DateUtils.h>

#ifdef VCMI_ANDROID
#include "lib/CAndroidVMHelper.h"
#endif

#ifdef VCMI_IOS
#	include "ios/utils.h"
#endif

#include <SDL3/SDL.h>

static constexpr Point heroes3Resolution = Point(800, 600);

/// SDL3 identifies displays by opaque ID's while our settings store a plain index
static SDL_DisplayID displayIndexToID(int displayIndex)
{
	int displaysCount = 0;
	SDL_DisplayID * displays = SDL_GetDisplays(&displaysCount);

	SDL_DisplayID result = 0;
	if (displays && displayIndex >= 0 && displayIndex < displaysCount)
		result = displays[displayIndex];

	SDL_free(displays);

	if (result == 0)
		result = SDL_GetPrimaryDisplay();

	return result;
}

[[maybe_unused]] static int displayIDToIndex(SDL_DisplayID displayID)
{
	int displaysCount = 0;
	SDL_DisplayID * displays = SDL_GetDisplays(&displaysCount);

	int result = -1;
	for (int i = 0; displays && i < displaysCount; ++i)
		if (displays[i] == displayID)
			result = i;

	SDL_free(displays);
	return result;
}

std::tuple<int, int> ScreenHandler::getSupportedScalingRange() const
{
	// H3 resolution, any resolution smaller than that is not correctly supported
	static constexpr Point minResolution = heroes3Resolution;
	// arbitrary limit on *downscaling*. Allow some downscaling, if requested by user. Should be generally limited to 100+ for all but few devices
	static constexpr double minimalScaling = 50;

	Point renderResolution = getRenderResolution();
	double reservedAreaWidth = settings["video"]["reservedWidth"].Float();
	Point availableResolution = Point(renderResolution.x * (1 - reservedAreaWidth), renderResolution.y);
	if(renderResolution.x < renderResolution.y) // reserved in portrait mode
		availableResolution = Point(renderResolution.x, renderResolution.y * (1 - reservedAreaWidth));

	double maximalScalingWidth = 100.0 * availableResolution.x / minResolution.x;
	double maximalScalingHeight = 100.0 * availableResolution.y / minResolution.y;
	double maximalScaling = std::min(maximalScalingWidth, maximalScalingHeight);

	return { minimalScaling, maximalScaling };
}

Rect ScreenHandler::convertLogicalPointsToWindow(const Rect & input) const
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	// SDL_GetRenderScale no longer reflects the logical presentation in SDL3, so let the
	// renderer convert - interface coordinates have to be in its (upscaled) space first.
	int scaling = getScalingFactor();

	float x0;
	float y0;
	float x1;
	float y1;

	SDL_RenderCoordinatesToWindow(renderer, input.x * scaling, input.y * scaling, &x0, &y0);
	SDL_RenderCoordinatesToWindow(renderer, (input.x + input.w) * scaling, (input.y + input.h) * scaling, &x1, &y1);

	Rect result;
	result.x = x0;
	result.y = y0;
	result.w = x1 - x0;
	result.h = y1 - y0;

	return result;
}

int ScreenHandler::getInterfaceScalingPercentage() const
{
	auto [minimalScaling, maximalScaling] = getSupportedScalingRange();

	int userScaling = settings["video"]["resolution"]["scaling"].Integer();

	if (userScaling == 0) // autodetection
	{
#ifdef VCMI_MOBILE
		// for mobiles - stay at maximum scaling unless we have large screen
		// might be better to check screen DPI / physical dimensions, but way more complex, and may result in different edge cases, e.g. chromebooks / tv's
		int preferredMinimalScaling = 200;
#else
		// for PC - avoid downscaling if possible
		int preferredMinimalScaling = 100;
#endif
		// prefer a little below maximum - to give space for extended UI
		int preferredMaximalScaling = maximalScaling * 10 / 12;
		userScaling = std::max(std::min(maximalScaling, preferredMinimalScaling), preferredMaximalScaling);
	}

	int scaling = std::clamp(userScaling, minimalScaling, maximalScaling);
	return scaling;
}

Point ScreenHandler::getPreferredLogicalResolution() const
{
	Point renderResolution = getRenderResolution();
	double reservedAreaWidth = settings["video"]["reservedWidth"].Float();

	int scaling = getInterfaceScalingPercentage();
	Point availableResolution = Point(renderResolution.x * (1 - reservedAreaWidth), renderResolution.y);
	if(renderResolution.x < renderResolution.y) // reserved in portrait mode
		availableResolution = Point(renderResolution.x, renderResolution.y * (1 - reservedAreaWidth));
	Point logicalResolution = availableResolution * 100.0 / scaling;

	// a window smaller than this in one axis keeps that axis at the limit, while the other
	// still follows the window - so the map gains space instead of the view being refused
	logicalResolution.x = std::max(logicalResolution.x, heroes3Resolution.x);
	logicalResolution.y = std::max(logicalResolution.y, heroes3Resolution.y);

	return logicalResolution;
}

int ScreenHandler::getScalingFactor() const
{
	switch (upscalingFilter)
	{
		case EUpscalingFilter::NONE: return 1;
		case EUpscalingFilter::XBRZ_2: return 2;
		case EUpscalingFilter::XBRZ_3: return 3;
		case EUpscalingFilter::XBRZ_4: return 4;
	}

	throw std::runtime_error("invalid upscaling filter");
}

Point ScreenHandler::getLogicalResolution() const
{
	return Point(screen->w, screen->h) / getScalingFactor();
}

Point ScreenHandler::getRenderResolution() const
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	assert(renderer != nullptr);

	Point result;
	// not the "current" size - that one reports the letterboxed logical area, not the window
	SDL_GetRenderOutputSize(renderer, &result.x, &result.y);

	return result;
}

Point ScreenHandler::getPreferredWindowResolution() const
{
	if (getPreferredWindowMode() == EWindowMode::FULLSCREEN_BORDERLESS_WINDOWED)
	{
		SDL_Rect bounds;
		if (SDL_GetDisplayBounds(displayIndexToID(getPreferredDisplayIndex()), &bounds))
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
		int result = displayIDToIndex(SDL_GetDisplayForWindow(mainWindow));
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
	// NOTE: SDL3 is always per-monitor DPI aware, so the Windows-specific
	// SDL_HINT_WINDOWS_DPI_AWARENESS of SDL2 has no equivalent here
	if(settings["video"]["allowPortrait"].Bool())
		SDL_SetHint(SDL_HINT_ORIENTATIONS, "Portrait PortraitUpsideDown LandscapeLeft LandscapeRight");
	else
		SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");

#ifdef VCMI_IOS
	if(!settings["general"]["ignoreMuteSwitch"].Bool())
		SDL_SetHint(SDL_HINT_AUDIO_CATEGORY, "AVAudioSessionCategoryAmbient");
#endif

	if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD))
	{
		logGlobal->error("Something was wrong: %s", SDL_GetError());
		exit(-1);
	}

	const auto & logCallback = [](void * userdata, int category, SDL_LogPriority priority, const char * message)
	{
		logGlobal->debug("SDL(category %d; priority %d) %s", category, priority, message);
	};

	SDL_SetLogOutputFunction(logCallback, nullptr);

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
			SDL_SetWindowFullscreen(mainWindow, false);

			SDL_DisplayID displayID = displayIndexToID(displayIndex);
			Point resolution = getPreferredWindowResolution();

			SDL_DisplayMode mode;
			if (SDL_GetClosestFullscreenDisplayMode(displayID, resolution.x, resolution.y, 0.0f, false, &mode))
				SDL_SetWindowFullscreenMode(mainWindow, &mode);

			SDL_SetWindowPosition(mainWindow, SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayID), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayID));
			SDL_SetWindowFullscreen(mainWindow, true);

			break;
		}
		case EWindowMode::FULLSCREEN_BORDERLESS_WINDOWED:
		{
			SDL_DisplayID displayID = displayIndexToID(displayIndex);
			// a null fullscreen mode means borderless fullscreen at desktop resolution
			SDL_SetWindowFullscreenMode(mainWindow, nullptr);
			SDL_SetWindowPosition(mainWindow, SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayID), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayID));
			SDL_SetWindowFullscreen(mainWindow, true);
			break;
		}
		case EWindowMode::WINDOWED:
		{
			Point resolution = getPreferredWindowResolution();
			SDL_SetWindowFullscreen(mainWindow, false);
			SDL_SetWindowSize(mainWindow, resolution.x, resolution.y);
			break;
		}
	}

	// SDL3 applies these asynchronously on some backends, so the caller would size the
	// screen buffers for the previous window - letterboxing them into the new one
	SDL_SyncWindow(mainWindow);
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

	// create first available renderer if no preferred one is set
	std::string preferredDriver = getPreferredRenderingDriver();
	GpuResources::get().setRenderer(SDL_CreateRenderer(mainWindow, preferredDriver.empty() ? nullptr : preferredDriver.c_str()));

	if(GpuResources::get().renderer() == nullptr)
	{
		const char * error = SDL_GetError();
		std::string messagePattern = "Failed to create SDL renderer. Reason: %s";
		std::string message = boost::str(boost::format(messagePattern) % error);
		handleFatalError(message, true);
	}

	SDL_SetRenderVSync(GpuResources::get().renderer(), settings["video"]["vsync"].Bool() ? 1 : SDL_RENDERER_VSYNC_DISABLED);

	selectUpscalingFilter();
	selectDownscalingFilter();

	logGlobal->info("Created renderer %s", SDL_GetRendererName(GpuResources::get().renderer()));
}

EUpscalingFilter ScreenHandler::loadUpscalingFilter() const
{
	static const std::map<std::string, EUpscalingFilter> upscalingFilterTypes =
	{
		{"auto", EUpscalingFilter::AUTO },
		{"none", EUpscalingFilter::NONE },
		{"xbrz2", EUpscalingFilter::XBRZ_2 },
		{"xbrz3", EUpscalingFilter::XBRZ_3 },
		{"xbrz4", EUpscalingFilter::XBRZ_4 }
	};

	auto filterName = settings["video"]["upscalingFilter"].String();
	auto filter = upscalingFilterTypes.count(filterName) ? upscalingFilterTypes.at(filterName) : EUpscalingFilter::AUTO;

	if (filter != EUpscalingFilter::AUTO)
		return filter;

	// else - autoselect
	Point outputResolution = getRenderResolution();
	Point logicalResolution = getPreferredLogicalResolution();

	float scaleX = static_cast<float>(outputResolution.x) / logicalResolution.x;
	float scaleY = static_cast<float>(outputResolution.x) / logicalResolution.x;
	float scaling = std::min(scaleX, scaleY);
	int systemMemoryMb = SDL_GetSystemRAM();

	if (scaling <= 1.001f)
		return EUpscalingFilter::NONE; // running at original resolution or even lower than that - no need for xbrz

	if (systemMemoryMb <= 4096)
		return EUpscalingFilter::NONE; // xbrz2 may use ~1.0 - 1.5 Gb of RAM and has notable CPU cost - avoid on low-spec hardware

	// Only using xbrz2 for autoselection.
	// Higher options may have high system requirements and should be only selected explicitly by player
	return EUpscalingFilter::XBRZ_2;
}

void ScreenHandler::selectUpscalingFilter()
{
	upscalingFilter	= loadUpscalingFilter();
	logGlobal->debug("Selected upscaling filter %d", static_cast<int>(upscalingFilter));
}

void ScreenHandler::selectDownscalingFilter()
{
	// SDL3 replaced the global SDL_HINT_RENDER_SCALE_QUALITY hint with a per-renderer default
	static const std::map<std::string, SDL_ScaleMode> scaleModes =
	{
		{"nearest", SDL_SCALEMODE_NEAREST },
		{"linear",  SDL_SCALEMODE_LINEAR },
		{"best",    SDL_SCALEMODE_LINEAR }
	};

	auto filterName = settings["video"]["downscalingFilter"].String();
	auto scaleMode = scaleModes.count(filterName) ? scaleModes.at(filterName) : SDL_SCALEMODE_LINEAR;

	SDL_SetDefaultTextureScaleMode(GpuResources::get().renderer(), scaleMode);
	logGlobal->debug("Selected downscaling filter %s", filterName);
}

void ScreenHandler::initializeScreenBuffers()
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	auto logicalSize = getPreferredLogicalResolution() * getScalingFactor();
	SDL_SetRenderLogicalPresentation(renderer, logicalSize.x, logicalSize.y, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	screen = SDL_CreateSurface(logicalSize.x, logicalSize.y, SDL_PIXELFORMAT_ARGB8888);
	if(nullptr == screen)
	{
		logGlobal->error("Unable to create surface %dx%d with %d bpp: %s", logicalSize.x, logicalSize.y, 32, SDL_GetError());
		throw std::runtime_error("Unable to create surface");
	}
	//No blending for screen itself. Required for proper cursor rendering.
	SDL_SetSurfaceBlendMode(screen, SDL_BLENDMODE_NONE);

	screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, logicalSize.x, logicalSize.y);

	if(nullptr == screenTexture)
	{
		logGlobal->error("Unable to create screen texture");
		logGlobal->error(SDL_GetError());
		throw std::runtime_error("Unable to create screen texture");
	}

	initializeLayerTextures(logicalSize);
	buffersRenderResolution = getRenderResolution();

	clearScreen();
}

SDL_Window * ScreenHandler::createWindowImpl(Point dimensions, uint64_t flags, bool center)
{
	SDL_DisplayID displayID = displayIndexToID(getPreferredDisplayIndex());
	int positionFlags = center ? SDL_WINDOWPOS_CENTERED_DISPLAY(displayID) : SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayID);

	// unlike SDL2, SDL3 rejects a window of zero size even in fullscreen modes
	if (dimensions.x <= 0 || dimensions.y <= 0)
	{
		SDL_Rect bounds;
		if (SDL_GetDisplayBounds(displayID, &bounds))
			dimensions = Point(bounds.w, bounds.h);
		else
			dimensions = heroes3Resolution;
	}

	// SDL3 no longer takes the position on creation, it has to be applied afterwards
	SDL_Window * result = SDL_CreateWindow(GameConstants::VCMI_PROJECT_NAME_VERSIONED, dimensions.x, dimensions.y, flags);

	if (result != nullptr)
		SDL_SetWindowPosition(result, positionFlags, positionFlags);

	return result;
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
			// with no fullscreen mode set, SDL_WINDOW_FULLSCREEN is borderless at desktop resolution
			return createWindowImpl(Point(), SDL_WINDOW_FULLSCREEN, false);

		case EWindowMode::WINDOWED:
			return createWindowImpl(dimensions, SDL_WINDOW_RESIZABLE, true);

		default:
			return nullptr;
	};
#endif

#ifdef VCMI_IOS
	SDL_SetHint(SDL_HINT_IOS_HIDE_HOME_INDICATOR, "1");
	SDL_SetHint(SDL_HINT_RETURN_KEY_HIDES_IME, "1");

	uint64_t windowFlags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	SDL_Window * result = createWindowImpl(Point(), windowFlags | SDL_WINDOW_METAL, false);

	if(result != nullptr)
		return result;

	logGlobal->warn("Metal unavailable, using OpenGLES");
	return createWindowImpl(Point(), windowFlags, false);
#endif

#ifdef VCMI_ANDROID
	// without the fullscreen flag SDL reports the window as leaving fullscreen once it is
	// created, and its Android backend answers that by showing the system bars again
	return createWindowImpl(Point(), SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN, false);
#endif
}

bool ScreenHandler::onScreenResize(bool keepWindowResolution)
{
	if (keepWindowResolution)
	{
		// SDL reports one window change as up to two events, so the second one asks to rebuild
		// buffers that already match the window - doing so would only flicker
		if (screen != nullptr && getRenderResolution() == buffersRenderResolution)
			return false;

		// the stored resolution is the windowed one, so a fullscreen window must not overwrite it
		if (getPreferredWindowMode() == EWindowMode::WINDOWED)
		{
			auto res = getRenderResolution();

			Settings video = settings.write["video"];
			video["resolution"]["width"].Integer() = res.x;
			video["resolution"]["height"].Integer() = res.y;
		}

		// Only recreate buffers (no window changes!)
		destroyScreenBuffers();
		initializeScreenBuffers();
	}
	else
	{
		// Apply settings change (may resize window / change fullscreen)
		recreateWindowAndScreenBuffers();
	}

	return true;
}

void ScreenHandler::validateSettings()
{
#ifndef VCMI_MOBILE
	{
		int displayIndex = settings["video"]["displayIndex"].Integer();
		int displaysCount = 0;
		SDL_free(SDL_GetDisplays(&displaysCount));

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

		const SDL_DisplayMode * mode = SDL_GetDesktopDisplayMode(displayIndexToID(displayIndex));

		if (mode != nullptr)
		{
			if(resolution.x > mode->w || resolution.y > mode->h)
			{
				Settings writer = settings.write["video"]["resolution"];
				writer["width"].Float() = mode->w;
				writer["height"].Float() = mode->h;
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
			const SDL_DisplayMode * mode = SDL_GetDesktopDisplayMode(displayIndexToID(getPreferredDisplayIndex()));

			if (mode != nullptr)
			{
				Settings writer = settings.write["video"]["resolution"];
				writer["width"].Float() = mode->w;
				writer["height"].Float() = mode->h;
			}
		}
	}
#endif
}

std::string ScreenHandler::getPreferredRenderingDriver() const
{
	std::string result;
	const JsonNode & video = settings["video"];

	int driversCount = SDL_GetNumRenderDrivers();
	std::string preferredDriverName = video["driver"].String();

	logGlobal->info("Found %d render drivers", driversCount);

	for(int it = 0; it < driversCount; it++)
	{
		const char * driver = SDL_GetRenderDriver(it);
		if (driver != nullptr)
		{
			std::string driverName(driver);

			if(!preferredDriverName.empty() && driverName == preferredDriverName)
			{
				result = driverName;
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
	if(nullptr != screen)
	{
		SDL_DestroySurface(screen);
		screen = nullptr;
	}

	if(nullptr != screenTexture)
	{
		SDL_DestroyTexture(screenTexture);
		screenTexture = nullptr;
	}

	if(nullptr != screenTarget)
	{
		SDL_DestroyTexture(screenTarget);
		screenTarget = nullptr;
	}

	for(SDL_Texture *& layer : layerTextures)
	{
		if(nullptr != layer)
		{
			SDL_DestroyTexture(layer);
			layer = nullptr;
		}
	}

	// the layers are gone, so a canvas handed out for one of them would draw nowhere
	gpuRenderingSupported = false;
}

void ScreenHandler::clearLayer(size_t index)
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	// the bottom layer is what everything else is composited onto, so it stays opaque;
	// the layers above it must start transparent to let it show through
	const bool bottom = index == 0;

	SDL_SetRenderTarget(renderer, layerTextures[index]);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, bottom ? 255 : 0);
	SDL_RenderClear(renderer);
}

void ScreenHandler::initializeLayerTextures(const Point & logicalSize)
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	// The software driver supports render targets too, but rasterizes them on the CPU -
	// going through textures there is slower than the surface blitting it would replace.
	const std::string driver = SDL_GetRendererName(renderer);
	if(driver == "software")
	{
		logGlobal->info("Software renderer in use - keeping the surface rendering path");
		return;
	}

	// every SDL3 renderer can render to a texture, so unlike SDL2 there is nothing to probe for
	for(size_t i = 0; i < layerTextures.size(); ++i)
	{
		layerTextures[i] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, logicalSize.x, logicalSize.y);

		if(nullptr == layerTextures[i])
		{
			logGlobal->error("Unable to create render target for GPU layer %d: %s", static_cast<int>(i), SDL_GetError());
			return;
		}

		SDL_SetTextureBlendMode(layerTextures[i], i == 0 ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
		clearLayer(i);
	}

	SDL_SetRenderTarget(renderer, nullptr);

	// the layers are drawn first and the software screen is blended over them, so the screen
	// texture must stop being opaque for them to show through its transparent holes
	SDL_SetTextureBlendMode(screenTexture, SDL_BLENDMODE_BLEND);

	screenTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, logicalSize.x, logicalSize.y);

	if(nullptr == screenTarget)
	{
		logGlobal->error("Unable to create the screen render target: %s", SDL_GetError());
		return;
	}

	// composited over the layers, so it must blend and start out fully transparent
	SDL_SetTextureBlendMode(screenTarget, SDL_BLENDMODE_BLEND);
	SDL_SetRenderTarget(renderer, screenTarget);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_SetRenderTarget(renderer, nullptr);

	gpuRenderingSupported = true;
	logGlobal->info("GPU rendering enabled, using driver '%s'", driver);
}

void ScreenHandler::destroyWindow()
{
	if(nullptr != GpuResources::get().renderer())
	{
		GpuResources::get().destroyRenderer();
	}

	if(nullptr != mainWindow)
	{
		SDL_DestroyWindow(mainWindow);
		mainWindow = nullptr;
	}
}

ScreenHandler::~ScreenHandler()
{
	if(settings["general"]["notifications"].Bool())
		NotificationHandler::destroy();

	destroyScreenBuffers();
	destroyWindow();
	SDL_Quit();
}

void ScreenHandler::clearScreen()
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

Canvas ScreenHandler::getScreenCanvas() const
{
	if(isGpuRenderingEnabled())
		return Canvas::createFromRenderTarget(screenTarget, getLogicalResolution(), CanvasScalingPolicy::AUTO);

	return Canvas::createFromSurface(screen, CanvasScalingPolicy::AUTO);
}

bool ScreenHandler::isGpuRenderingEnabled() const
{
	// no GPU path with the software driver, nor with a colour scheme - that needs a shader
	return gpuRenderingSupported && colorScheme == ColorScheme::NONE;
}

Canvas ScreenHandler::getLayerCanvas(GpuRenderLayer layer)
{
	const size_t index = static_cast<size_t>(layer);

	layerActive.at(index) = true;
	layerReleasedMask &= ~(1u << index);

	return Canvas::createFromRenderTarget(layerTextures.at(index), getLogicalResolution(), CanvasScalingPolicy::AUTO);
}

void ScreenHandler::releaseLayer(GpuRenderLayer layer)
{
	layerReleasedMask |= 1u << static_cast<size_t>(layer);
}

void ScreenHandler::clearReleasedLayers()
{
	// Windows are deactivated when closed and when merely covered, so this runs before the
	// redraw and getLayerCanvas() takes the request back if the owner draws again.
	const uint32_t released = layerReleasedMask.exchange(0);

	if(released == 0)
		return;

	for(size_t i = 0; i < layerTextures.size(); ++i)
	{
		if(!layerTextures[i] || (released & (1u << i)) == 0)
			continue;

		clearLayer(i);
		layerActive[i] = false;
	}

	SDL_SetRenderTarget(GpuResources::get().renderer(), nullptr);
}

Canvas ScreenHandler::createOffscreenCanvas(const Point & size) const
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	// the software driver rasterizes a render target on the CPU, which is slower than the
	// surface blitting it would replace - the same reason the layers stay on surfaces there
	if(!isGpuRenderingEnabled())
		return Canvas(size, CanvasScalingPolicy::AUTO);

	const Point pixels = size * getScalingFactor();
	SDL_Texture * target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, pixels.x, pixels.y);

	if(!target)
	{
		logGlobal->error("Failed to create %dx%d render target: %s - falling back to software", pixels.x, pixels.y, SDL_GetError());
		return Canvas(size, CanvasScalingPolicy::AUTO);
	}

	SDL_SetTextureBlendMode(target, SDL_BLENDMODE_BLEND);

	// a fresh render target holds undefined contents - start it transparent so that
	// blending the first sprites onto it produces the same result as the surface path
	SDL_Texture * previousTarget = SDL_GetRenderTarget(renderer);
	SDL_SetRenderTarget(renderer, target);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_SetRenderTarget(renderer, previousTarget);

	return Canvas::createOwningRenderTarget(target, size, CanvasScalingPolicy::AUTO);
}

void ScreenHandler::updateScreenTexture()
{
	// A window change SDL applied late leaves the buffers sized for the previous window,
	// which the renderer then letterboxes into the current one. Events alone do not catch
	// every such case, so the size is reconciled once per frame instead.
	if(screen != nullptr && getRenderResolution() != buffersRenderResolution)
	{
		ENGINE->onScreenResize(true, true);
		return;
	}

	// windows drew straight into screenTarget, so there is no surface to upload
	if(isGpuRenderingEnabled())
		return;

	if(colorScheme == ColorScheme::NONE)
	{
		SDL_UpdateTexture(screenTexture, nullptr, screen->pixels, screen->pitch);
		return;
	}

	SDL_Surface * screenScheme = SDL_DuplicateSurface(screen);
	if(colorScheme == ColorScheme::GRAYSCALE)
		CSDL_Ext::convertToGrayscale(screenScheme, Rect(0, 0, screen->w, screen->h));
	else if(colorScheme == ColorScheme::H2_SCHEME)
		CSDL_Ext::convertToH2Scheme(screenScheme, Rect(0, 0, screen->w, screen->h));
	SDL_UpdateTexture(screenTexture, nullptr, screenScheme->pixels, screenScheme->pitch);
	SDL_DestroySurface(screenScheme);
}

void ScreenHandler::flushRenderCommands()
{
	SDL_FlushRenderer(GpuResources::get().renderer());
}

void ScreenHandler::presentScreenTexture()
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	// a layer may still be bound from rendering into it
	SDL_SetRenderTarget(renderer, nullptr);

	{
		GpuResources::get().processPendingTextureDestruction();
	}

	// the draw color is left over from whatever was rendered last, and this clear also
	// covers the letterbox bars around the reserved area
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	for(size_t i = 0; i < layerTextures.size(); ++i)
		if(layerTextures[i] && layerActive[i])
			SDL_RenderTexture(renderer, layerTextures[i], nullptr, nullptr);

	SDL_RenderTexture(renderer, isGpuRenderingEnabled() ? screenTarget : screenTexture, nullptr, nullptr);
	ENGINE->cursor().render();
	SDL_RenderPresent(renderer);
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

	int modesCount = 0;
	SDL_DisplayMode ** modes = SDL_GetFullscreenDisplayModes(displayIndexToID(displayIndex), &modesCount);

	for (int i = 0; modes && i < modesCount; ++i)
		result.push_back(Point(modes[i]->w, modes[i]->h));

	SDL_free(modes);

	std::ranges::sort(result, [](const auto & left, const auto & right)
	{
		return left.x * left.y < right.x * right.y;
	});

	// erase potential duplicates, e.g. resolutions with different framerate / bits per pixel
	result.erase(std::ranges::unique(result).end(), result.end());

	return result;
}

bool ScreenHandler::hasFocus()
{
	SDL_WindowFlags flags = SDL_GetWindowFlags(mainWindow);
	return flags & SDL_WINDOW_INPUT_FOCUS;
}

void ScreenHandler::setColorScheme(ColorScheme scheme)
{
	if(colorScheme == scheme)
		return;

	colorScheme = scheme;

	// this switches the whole client between the GPU and the surface path, so anything
	// already drawn into a layer has to be dropped and repainted from scratch
	for(size_t i = 0; i < layerTextures.size(); ++i)
		releaseLayer(static_cast<GpuRenderLayer>(i));

	ENGINE->windows().totalRedraw();
}

void ScreenHandler::screenShot() const
{
	const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / "screenshots";
	boost::filesystem::create_directories(outPath);
	const boost::filesystem::path filePath = outPath / ("screenshot-" + vstd::getDateTimeISO8601Basic(std::time(nullptr)) + ".png");
	auto img = std::make_shared<SDLImageShared>(screen);
	img->exportBitmap(filePath, nullptr);
	MetaString txt;
	txt.appendTextID("vcmi.client.screenShot");
	txt.replaceRawString(filePath.string());
	if(GAME->interface())
		GAME->server().getGameChat().sendMessageGameplay(txt.toString(&GAME->translator()));
}
