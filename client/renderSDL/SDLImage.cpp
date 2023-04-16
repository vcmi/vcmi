/*
 * SDLImage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "SDLImage.h"

#include "SDLImageLoader.h"
#include "SDL_Extensions.h"

#include "../render/ColorFilter.h"
#include "../render/CBitmapHandler.h"
#include "../render/CDefFile.h"
#include "../render/Graphics.h"

#include "../../lib/JsonNode.h"

#include <SDL_surface.h>

class SDLImageLoader;

std::shared_ptr<IImage> IImage::createFromFile( const std::string & path )
{
	return createFromFile(path, EImageBlitMode::ALPHA);
}

std::shared_ptr<IImage> IImage::createFromFile( const std::string & path, EImageBlitMode mode )
{
	return std::shared_ptr<IImage>(new SDLImage(path, mode));
}

std::shared_ptr<IImage> IImage::createFromSurface( SDL_Surface * source )
{
	return std::shared_ptr<IImage>(new SDLImage(source, EImageBlitMode::ALPHA));
}

IImage::IImage() = default;
IImage::~IImage() = default;

int IImage::width() const
{
	return dimensions().x;
}

int IImage::height() const
{
	return dimensions().y;
}

SDLImage::SDLImage(CDefFile * data, size_t frame, size_t group)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr)
{
	SDLImageLoader loader(this);
	data->loadFrame(frame, group, loader);

	savePalette();
	setBlitMode(EImageBlitMode::ALPHA);
}

SDLImage::SDLImage(SDL_Surface * from, EImageBlitMode mode)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr)
{
	surf = from;
	if (surf == nullptr)
		return;

	savePalette();
	setBlitMode(mode);

	surf->refcount++;
	fullSize.x = surf->w;
	fullSize.y = surf->h;
}

SDLImage::SDLImage(const JsonNode & conf, EImageBlitMode mode)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr)
{
	std::string filename = conf["file"].String();

	surf = BitmapHandler::loadBitmap(filename);

	if(surf == nullptr)
		return;

	savePalette();
	setBlitMode(mode);

	const JsonNode & jsonMargins = conf["margins"];

	margins.x = static_cast<int>(jsonMargins["left"].Integer());
	margins.y = static_cast<int>(jsonMargins["top"].Integer());

	fullSize.x = static_cast<int>(conf["width"].Integer());
	fullSize.y = static_cast<int>(conf["height"].Integer());

	if(fullSize.x == 0)
	{
		fullSize.x = margins.x + surf->w + (int)jsonMargins["right"].Integer();
	}

	if(fullSize.y == 0)
	{
		fullSize.y = margins.y + surf->h + (int)jsonMargins["bottom"].Integer();
	}
}

SDLImage::SDLImage(std::string filename, EImageBlitMode mode)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr)
{
	surf = BitmapHandler::loadBitmap(filename);

	if(surf == nullptr)
	{
		logGlobal->error("Error: failed to load image %s", filename);
		return;
	}
	else
	{
		savePalette();
		setBlitMode(mode);
		fullSize.x = surf->w;
		fullSize.y = surf->h;
	}
}

void SDLImage::draw(SDL_Surface *where, int posX, int posY, const Rect *src) const
{
	if(!surf)
		return;

	Rect destRect(posX, posY, surf->w, surf->h);
	draw(where, &destRect, src);
}

void SDLImage::draw(SDL_Surface* where, const Rect * dest, const Rect* src) const
{
	if (!surf)
		return;

	Rect sourceRect(0, 0, surf->w, surf->h);

	Point destShift(0, 0);

	if(src)
	{
		if(src->x < margins.x)
			destShift.x += margins.x - src->x;

		if(src->y < margins.y)
			destShift.y += margins.y - src->y;

		sourceRect = Rect(*src).intersect(Rect(margins.x, margins.y, surf->w, surf->h));

		sourceRect -= margins;
	}
	else
		destShift = margins;

	if(dest)
		destShift += dest->topLeft();

	uint8_t perSurfaceAlpha;
	if (SDL_GetSurfaceAlphaMod(surf, &perSurfaceAlpha) != 0)
		logGlobal->error("SDL_GetSurfaceAlphaMod faied! %s", SDL_GetError());

	if(surf->format->BitsPerPixel == 8 && perSurfaceAlpha == SDL_ALPHA_OPAQUE && blitMode == EImageBlitMode::ALPHA)
	{
		CSDL_Ext::blit8bppAlphaTo24bpp(surf, sourceRect, where, destShift);
	}
	else
	{
		CSDL_Ext::blitSurface(surf, sourceRect, where, destShift);
	}
}

std::shared_ptr<IImage> SDLImage::scaleFast(const Point & size) const
{
	float scaleX = float(size.x) / width();
	float scaleY = float(size.y) / height();

	auto scaled = CSDL_Ext::scaleSurfaceFast(surf, (int)(surf->w * scaleX), (int)(surf->h * scaleY));

	if (scaled->format && scaled->format->palette) // fix color keying, because SDL loses it at this point
		CSDL_Ext::setColorKey(scaled, scaled->format->palette->colors[0]);
	else if(scaled->format && scaled->format->Amask)
		SDL_SetSurfaceBlendMode(scaled, SDL_BLENDMODE_BLEND);//just in case
	else
		CSDL_Ext::setDefaultColorKey(scaled);//just in case

	auto * ret = new SDLImage(scaled, EImageBlitMode::ALPHA);

	ret->fullSize.x = (int) round((float)fullSize.x * scaleX);
	ret->fullSize.y = (int) round((float)fullSize.y * scaleY);

	ret->margins.x = (int) round((float)margins.x * scaleX);
	ret->margins.y = (int) round((float)margins.y * scaleY);

	// erase our own reference
	SDL_FreeSurface(scaled);

	return std::shared_ptr<IImage>(ret);
}

void SDLImage::exportBitmap(const boost::filesystem::path& path) const
{
	SDL_SaveBMP(surf, path.string().c_str());
}

void SDLImage::playerColored(PlayerColor player)
{
	graphics->blueToPlayersAdv(surf, player);
}

void SDLImage::setAlpha(uint8_t value)
{
	CSDL_Ext::setAlpha (surf, value);
	if (value != 255)
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_BLEND);
}

void SDLImage::setBlitMode(EImageBlitMode mode)
{
	blitMode = mode;

	if (blitMode != EImageBlitMode::OPAQUE && surf->format->Amask != 0)
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_BLEND);
	else
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);
}

void SDLImage::setFlagColor(PlayerColor player)
{
	if(player < PlayerColor::PLAYER_LIMIT || player==PlayerColor::NEUTRAL)
		CSDL_Ext::setPlayerColor(surf, player);
}

bool SDLImage::isTransparent(const Point & coords) const
{
	return CSDL_Ext::isTransparent(surf, coords.x, coords.y);
}

Point SDLImage::dimensions() const
{
	return fullSize;
}

void SDLImage::horizontalFlip()
{
	margins.y = fullSize.y - surf->h - margins.y;

	//todo: modify in-place
	SDL_Surface * flipped = CSDL_Ext::horizontalFlip(surf);
	SDL_FreeSurface(surf);
	surf = flipped;
}

void SDLImage::verticalFlip()
{
	margins.x = fullSize.x - surf->w - margins.x;

	//todo: modify in-place
	SDL_Surface * flipped = CSDL_Ext::verticalFlip(surf);
	SDL_FreeSurface(surf);
	surf = flipped;
}

// Keep the original palette, in order to do color switching operation
void SDLImage::savePalette()
{
	// For some images that don't have palette, skip this
	if(surf->format->palette == nullptr)
		return;

	if(originalPalette == nullptr)
		originalPalette = SDL_AllocPalette(DEFAULT_PALETTE_COLORS);

	SDL_SetPaletteColors(originalPalette, surf->format->palette->colors, 0, DEFAULT_PALETTE_COLORS);
}

void SDLImage::shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove)
{
	if(surf->format->palette)
	{
		std::vector<SDL_Color> shifterColors(colorsToMove);

		for(uint32_t i=0; i<colorsToMove; ++i)
		{
			shifterColors[(i+distanceToMove)%colorsToMove] = originalPalette->colors[firstColorID + i];
		}
		CSDL_Ext::setColors(surf, shifterColors.data(), firstColorID, colorsToMove);
	}
}

void SDLImage::adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask)
{
	if(originalPalette == nullptr)
		return;

	SDL_Palette* palette = surf->format->palette;

	// Note: here we skip first colors in the palette that are predefined in H3 images
	for(int i = 0; i < palette->ncolors; i++)
	{
		if(i < std::numeric_limits<uint32_t>::digits && ((colorsToSkipMask >> i) & 1) == 1)
			continue;

		palette->colors[i] = shifter.shiftColor(originalPalette->colors[i]);
	}
}

void SDLImage::resetPalette()
{
	if(originalPalette == nullptr)
		return;
	
	// Always keept the original palette not changed, copy a new palette to assign to surface
	SDL_SetPaletteColors(surf->format->palette, originalPalette->colors, 0, originalPalette->ncolors);
}

void SDLImage::resetPalette( int colorID )
{
	if(originalPalette == nullptr)
		return;

	// Always keept the original palette not changed, copy a new palette to assign to surface
	SDL_SetPaletteColors(surf->format->palette, originalPalette->colors + colorID, colorID, 1);
}

void SDLImage::setSpecialPallete(const IImage::SpecialPalette & specialPalette, uint32_t colorsToSkipMask)
{
	if(surf->format->palette)
	{
		size_t last = std::min<size_t>(specialPalette.size(), surf->format->palette->ncolors);

		for (size_t i = 0; i < last; ++i)
		{
			if(i < std::numeric_limits<uint32_t>::digits && ((colorsToSkipMask >> i) & 1) == 1)
				surf->format->palette->colors[i] = specialPalette[i];
		}
	}
}

SDLImage::~SDLImage()
{
	SDL_FreeSurface(surf);

	if(originalPalette != nullptr)
	{
		SDL_FreePalette(originalPalette);
		originalPalette = nullptr;
	}
}

