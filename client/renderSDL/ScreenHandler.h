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

struct SDL_Texture;
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Surface;

#include "../../lib/Point.h"
#include "../render/IScreenHandler.h"

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

/// This class is responsible for management of game window and its main rendering surface
class ScreenHandler final : public IScreenHandler
{
	/// Dimensions of target surfaces/textures, this value is what game logic views as screen size
	Point getPreferredLogicalResolution() const;

	/// Dimensions of output window, if different from logicalResolution SDL will perform scaling
	/// This value is what player views as window size
	Point getPreferredWindowResolution() const;

	/// Dimensions of render output, usually same as window size except for high-DPI screens on macOS / iOS
	Point getActualRenderResolution() const;

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
public:

	/// Creates and initializes screen, window and SDL state
	ScreenHandler();

	/// Updates and potentially recreates target screen to match selected fullscreen status
	void onScreenResize() final;

	/// De-initializes and destroys screen, window and SDL state
	void close() final;

	/// Fills screen with black color, erasing any existing content
	void clearScreen() final;

	std::vector<Point> getSupportedResolutions() const final;
	std::vector<Point> getSupportedResolutions(int displayIndex) const;
	std::tuple<int, int> getSupportedScalingRange() const final;
	Rect convertLogicalPointsToWindow(const Rect & input) const final;
};
