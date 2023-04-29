/*
 * WindowHandler.h, part of VCMI engine
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

enum class EWindowMode
{
	// game runs in a window that covers part of the screen
	WINDOWED,
	// game runs in a 'window' that always covers entire screen and uses unmodified desktop resolution
	// The only mode that is available on mobile devices
	FULLSCREEN_WINDOWED,
	// game runs in a fullscreen mode with resolution selected by player
	FULLSCREEN_TRUE
};

/// This class is responsible for management of game window and its main rendering surface
class WindowHandler
{
	/// Dimensions of target surfaces/textures, this value is what game logic views as screen size
	Point getPreferredLogicalResolution() const;

	/// Dimensions of output window, if different from logicalResolution SDL will perform scaling
	/// This value is what player views as window size
	Point getPreferredRenderingResolution() const;

	EWindowMode getPreferredWindowMode() const;

	/// Returns index of display on which window should be created
	int getPreferredDisplayIndex() const;

	/// Returns index of rendering driver preferred by player or -1 if no preference
	int getPreferredRenderingDriver() const;

	/// Creates SDL window with specified parameters
	SDL_Window * createWindowImpl(Point dimensions, int displayIndex, int flags, bool center);

	/// Creates SDL window using OS-specific settings & user-specific config
	SDL_Window * createWindow();

	void initializeRenderer();
	void initializeScreen();
	void updateFullscreenState();

	bool recreateWindow();
	void destroyScreen();
	void destroyWindow();

	void validateSettings();
public:

	/// Creates and initializes screen, window and SDL state
	WindowHandler();

	/// Updates and potentially recreates target screen to match selected fullscreen status
	void onFullscreenChanged();

	/// De-initializes and destroys screen, window and SDL state
	void close();
};
