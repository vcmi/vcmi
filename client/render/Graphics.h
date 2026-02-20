/*
 * Graphics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/constants/NumericConstants.h"
#include "../lib/constants/EntityIdentifiers.h"
#include "../lib/Color.h"

struct SDL_Palette;

/// Handles fonts, hero images, town images, various graphics
class Graphics
{
	void initializeBattleGraphics();
	void loadPaletteAndColors();

public:
	using PlayerPalette = std::array<ColorRGBA, 32>;

	//various graphics
	std::array<ColorRGBA, 8> playerColors;
	std::array<PlayerPalette, 8> playerColorPalette; //palette to make interface colors good - array of size [256]

	PlayerPalette neutralColorPalette;
	ColorRGBA neutralColor;

	//for battles
	std::map< int, std::vector < std::string > > battleACToDef; //maps AC format to vector of appropriate def names

	//functions
	Graphics();

	void setPlayerPalette(SDL_Palette * sur, PlayerColor player);
	void setPlayerFlagColor(SDL_Palette * sur, PlayerColor player);
};

extern Graphics * graphics;
