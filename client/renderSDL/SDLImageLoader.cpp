/*
 * SDLImageLoader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "SDLImageLoader.h"

#include "SDLImage.h"

#include "../../lib/Point.h"

#include <SDL_surface.h>

SDLImageLoader::SDLImageLoader(SDLImage * Img):
	image(Img),
	lineStart(nullptr),
	position(nullptr)
{
}

void SDLImageLoader::init(Point SpriteSize, Point Margins, Point FullSize, SDL_Color *pal)
{
	//Init image
	image->surf = SDL_CreateRGBSurface(0, SpriteSize.x, SpriteSize.y, 8, 0, 0, 0, 0);
	image->margins  = Margins;
	image->fullSize = FullSize;

	//Prepare surface
	SDL_Palette * p = SDL_AllocPalette(SDLImage::DEFAULT_PALETTE_COLORS);
	SDL_SetPaletteColors(p, pal, 0, SDLImage::DEFAULT_PALETTE_COLORS);
	SDL_SetSurfacePalette(image->surf, p);
	SDL_FreePalette(p);

	SDL_LockSurface(image->surf);
	lineStart = position = (ui8*)image->surf->pixels;
}

inline void SDLImageLoader::load(size_t size, const ui8 * data)
{
	if (size)
	{
		memcpy((void *)position, data, size);
		position += size;
	}
}

inline void SDLImageLoader::load(size_t size, ui8 color)
{
	if (size)
	{
		memset((void *)position, color, size);
		position += size;
	}
}

inline void SDLImageLoader::endLine()
{
	lineStart += image->surf->pitch;
	position = lineStart;
}

SDLImageLoader::~SDLImageLoader()
{
	SDL_UnlockSurface(image->surf);
	SDL_SetColorKey(image->surf, SDL_TRUE, 0);
	//TODO: RLE if compressed and bpp>1
}

