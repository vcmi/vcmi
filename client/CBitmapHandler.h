#pragma once



struct SDL_Surface;

/*
 * CBitmapHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Struct which stands for a simple rgba palette
struct BMPPalette
{
	ui8 R,G,B,F;
};

namespace BitmapHandler
{
	SDL_Surface * loadH3PCX(ui8 * data, size_t size);
	//Load file from specific LOD
	SDL_Surface * loadBitmapFromDir(std::string path, std::string fname, bool setKey=true);
	//Load file from any LODs
	SDL_Surface * loadBitmap(std::string fname, bool setKey=true);
}
