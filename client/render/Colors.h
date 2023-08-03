/*
 * ICursor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct SDL_Color;

/**
 * The colors class defines color constants of type SDL_Color.
 */
class Colors
{
public:
	/** the h3 yellow color, typically used for headlines */
	static const SDL_Color YELLOW;

	/** the standard h3 white color */
	static const SDL_Color WHITE;

	/** the metallic gold color used mostly as a border around buttons */
	static const SDL_Color METALLIC_GOLD;

	/** green color used for in-game console */
	static const SDL_Color GREEN;

	/** the h3 orange color, used for blocked buttons */
	static const SDL_Color ORANGE;

	/** the h3 bright yellow color, used for selection border */
	static const SDL_Color BRIGHT_YELLOW;

	/** default key color for all 8 & 24 bit graphics */
	static const SDL_Color DEFAULT_KEY_COLOR;

	/// Selected creature card
	static const SDL_Color RED;

	/// Minimap border
	static const SDL_Color PURPLE;

	static const SDL_Color CYAN;

	static const SDL_Color BLACK;

	static const SDL_Color TRANSPARENCY;
};
