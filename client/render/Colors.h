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

#include "../../lib/Color.h"

/**
 * The colors class defines color constants of type ColorRGBA.
 */
class Colors
{
public:
	/** the h3 yellow color, typically used for headlines */
	static const ColorRGBA YELLOW;

	/** the standard h3 white color */
	static const ColorRGBA WHITE;

	/** the metallic gold color used mostly as a border around buttons */
	static const ColorRGBA METALLIC_GOLD;

	/** green color used for in-game console */
	static const ColorRGBA GREEN;

	/** the h3 orange color, used for blocked buttons */
	static const ColorRGBA ORANGE;

	/** the h3 bright yellow color, used for selection border */
	static const ColorRGBA BRIGHT_YELLOW;

	/** default key color for all 8 & 24 bit graphics */
	static const ColorRGBA DEFAULT_KEY_COLOR;

	/// Selected creature card
	static const ColorRGBA RED;

	/// Minimap border
	static const ColorRGBA PURPLE;

	static const ColorRGBA CYAN;

	static const ColorRGBA BLACK;

	static const ColorRGBA TRANSPARENCY;
};
