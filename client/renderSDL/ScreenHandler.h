/*
 * ScreenHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/Point.h"
#include "../render/IScreenHandler.h"

struct SDL_Texture;
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Surface;

enum class EWindowMode
{
	// game runs in a window that covers part of the screen
	WINDOWED,
	// game runs in a 'window' that always covers entire screen and uses unmodified desktop resolution
	// The only mode that is available on mobile devices
	FULLSCREEN_BORDERLESS_WINDOWED,
	// game runs in a fullscreen mode with resolution selected by player
	FULLSCREEN_EXCLUSIVE
};

enum class EUpscalingFilter
{
	AUTO, // used only for loading from config, replaced with autoselected value on init
	NONE,
	//BILINEAR, // TODO?
	//BICUBIC, // TODO?
	XBRZ_2,
	XBRZ_3,
	XBRZ_4,
	// NOTE: xbrz also provides x5 and x6 filters, but those would require high-end gaming PC's due to huge memory usage with no visible gain
};

/// This class is responsible for management of game window and its main rendering surface
class ScreenHandler final : public IScreenHandler
{
	SDL_Window * mainWindow = nullptr;
	SDL_Texture * screenTexture = nullptr;
	SDL_Surface * screen = nullptr;

	EUpscalingFilter upscalingFilter = EUpscalingFilter::AUTO;
	ColorScheme colorScheme = ColorScheme::NONE;

	/// Dimensions of target surfaces/textures, this value is what game logic views as screen size
	Point getPreferredLogicalResolution() const;

	/// Dimensions of output window, if different from logicalResolution SDL will perform scaling
	/// This value is what player views as window size
	Point getPreferredWindowResolution() const;

	EWindowMode getPreferredWindowMode() const;

	/// Returns index of display on which window should be created
	int getPreferredDisplayIndex() const;

	/// Returns index of rendering driver preferred by player or -1 if no preference
	int getPreferredRenderingDriver() const;

	/// Creates SDL window with specified parameters
	SDL_Window * createWindowImpl(Point dimensions, int flags, bool center);

	/// Creates SDL window using OS-specific settings & user-specific config
	SDL_Window * createWindow();

	/// Manages window and SDL renderer
	void initializeWindow();
	void destroyWindow();

	/// Manages surfaces & textures used for
	void initializeScreenBuffers();
	void destroyScreenBuffers();

	/// Updates state (e.g. position) of game window after resolution/fullscreen change
	void updateWindowState();

	/// Initializes or reiniitalizes all screen state
	void recreateWindowAndScreenBuffers();

	/// Performs validation of settings and updates them to valid values if necessary
	void validateSettings();

	EUpscalingFilter loadUpscalingFilter() const;

	void selectDownscalingFilter();
	void selectUpscalingFilter();
public:

	/// Creates and initializes screen, window and SDL state
	ScreenHandler();
	~ScreenHandler();

	/// Updates and potentially recreates target screen to match selected fullscreen status
	bool onScreenResize(bool keepWindowResolution) final;

	/// Fills screen with black color, erasing any existing content
	void clearScreen() final;

	/// Dimensions of render output, usually same as window size except for high-DPI screens on macOS / iOS
	Point getRenderResolution() const final;

	/// Window has focus
	bool hasFocus() final;

	Point getLogicalResolution() const final;

	int getScalingFactor() const final;

	int getInterfaceScalingPercentage() const final;

	Canvas getScreenCanvas() const final;
	void updateScreenTexture() final;
	void presentScreenTexture() final;

	std::vector<Point> getSupportedResolutions() const final;
	std::vector<Point> getSupportedResolutions(int displayIndex) const;
	std::tuple<int, int> getSupportedScalingRange() const final;
	Rect convertLogicalPointsToWindow(const Rect & input) const final;

	void setColorScheme(ColorScheme filter) final;
};
