/*
 * CBitmapHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/filesystem/ResourcePath.h"

struct SDL_Surface;

namespace BitmapHandler
{
	//Load file from /DATA or /SPRITES
	SDL_Surface * loadBitmap(const ImagePath & fname);
}
