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

#include <SDL_surface.h>

class SDLImageLoader;

int IImage::width() const
{
	return dimensions().x;
}

int IImage::height() const
{
	return dimensions().y;
}

SDLImageConst::SDLImageConst(CDefFile * data, size_t frame, size_t group)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr)
{
	SDLImageLoader loader(this);
	data->loadFrame(frame, group, loader);

	savePalette();
}

SDLImageConst::SDLImageConst(SDL_Surface * from)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr)
{
	surf = from;
	if (surf == nullptr)
		return;

	savePalette();

	surf->refcount++;
	fullSize.x = surf->w;
	fullSize.y = surf->h;
}

SDLImageConst::SDLImageConst(const ImagePath & filename)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr)
{
	surf = BitmapHandler::loadBitmap(filename);

	if(surf == nullptr)
	{
		logGlobal->error("Error: failed to load image %s", filename.getOriginalName());
		return;
	}
	else
	{
		savePalette();
		fullSize.x = surf->w;
		fullSize.y = surf->h;
	}
}


void SDLImageConst::draw(SDL_Surface * where, SDL_Palette * palette, const Point & dest, const Rect * src, uint8_t alpha, EImageBlitMode mode) const
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

	destShift += dest;

	SDL_SetSurfaceAlphaMod(surf, alpha);

	if (alpha != SDL_ALPHA_OPAQUE || (mode != EImageBlitMode::OPAQUE && surf->format->Amask != 0))
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_BLEND);
	else
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);

	if (palette && surf->format->palette)
		SDL_SetSurfacePalette(surf, palette);

	if(surf->format->palette && mode == EImageBlitMode::ALPHA)
	{
		CSDL_Ext::blit8bppAlphaTo24bpp(surf, sourceRect, where, destShift, alpha);
	}
	else
	{
		CSDL_Ext::blitSurface(surf, sourceRect, where, destShift);
	}
}

const SDL_Palette * SDLImageConst::getPalette() const
{
	if (originalPalette == nullptr)
		throw std::runtime_error("Palette not found!");

	return originalPalette;
}

std::shared_ptr<SDLImageConst> SDLImageConst::scaleFast(const Point & size) const
{
	float scaleX = float(size.x) / dimensions().x;
	float scaleY = float(size.y) / dimensions().y;

	auto scaled = CSDL_Ext::scaleSurface(surf, (int)(surf->w * scaleX), (int)(surf->h * scaleY));

	if (scaled->format && scaled->format->palette) // fix color keying, because SDL loses it at this point
		CSDL_Ext::setColorKey(scaled, scaled->format->palette->colors[0]);
	else if(scaled->format && scaled->format->Amask)
		SDL_SetSurfaceBlendMode(scaled, SDL_BLENDMODE_BLEND);//just in case
	else
		CSDL_Ext::setDefaultColorKey(scaled);//just in case

	auto ret = std::make_shared<SDLImageConst>(scaled);

	ret->fullSize.x = (int) round((float)fullSize.x * scaleX);
	ret->fullSize.y = (int) round((float)fullSize.y * scaleY);

	ret->margins.x = (int) round((float)margins.x * scaleX);
	ret->margins.y = (int) round((float)margins.y * scaleY);

	// erase our own reference
	SDL_FreeSurface(scaled);

	return ret;
}

void SDLImageConst::exportBitmap(const boost::filesystem::path& path) const
{
	SDL_SaveBMP(surf, path.string().c_str());
}

void SDLImageIndexed::playerColored(PlayerColor player)
{
	graphics->setPlayerPalette(currentPalette, player);
}

void SDLImageIndexed::setFlagColor(PlayerColor player)
{
	if(player.isValidPlayer() || player==PlayerColor::NEUTRAL)
		graphics->setPlayerFlagColor(currentPalette, player);
}

bool SDLImageConst::isTransparent(const Point & coords) const
{
	if (surf)
		return CSDL_Ext::isTransparent(surf, coords.x, coords.y);
	else
		return true;
}

Point SDLImageConst::dimensions() const
{
	return fullSize;
}

std::shared_ptr<IImage> SDLImageConst::createImageReference(EImageBlitMode mode)
{
	if (surf && surf->format->palette)
		return std::make_shared<SDLImageIndexed>(shared_from_this(), mode);
	else
		return std::make_shared<SDLImageRGB>(shared_from_this(), mode);
}

std::shared_ptr<IConstImage> SDLImageConst::horizontalFlip() const
{
	SDL_Surface * flipped = CSDL_Ext::horizontalFlip(surf);
	auto ret = std::make_shared<SDLImageConst>(flipped);
	ret->fullSize = fullSize;
	ret->margins.x = margins.x;
	ret->margins.y = fullSize.y - surf->h - margins.y;
	ret->fullSize = fullSize;

	return ret;
}

std::shared_ptr<IConstImage> SDLImageConst::verticalFlip() const
{
	SDL_Surface * flipped = CSDL_Ext::verticalFlip(surf);
	auto ret = std::make_shared<SDLImageConst>(flipped);
	ret->fullSize = fullSize;
	ret->margins.x = fullSize.x - surf->w - margins.x;
	ret->margins.y = margins.y;
	ret->fullSize = fullSize;

	return ret;
}

// Keep the original palette, in order to do color switching operation
void SDLImageConst::savePalette()
{
	// For some images that don't have palette, skip this
	if(surf->format->palette == nullptr)
		return;

	if(originalPalette == nullptr)
		originalPalette = SDL_AllocPalette(surf->format->palette->ncolors);

	SDL_SetPaletteColors(originalPalette, surf->format->palette->colors, 0, surf->format->palette->ncolors);
}

void SDLImageIndexed::shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove)
{
	const SDL_Palette * originalPalette = image->getPalette();

	std::vector<SDL_Color> shifterColors(colorsToMove);

	for(uint32_t i=0; i<colorsToMove; ++i)
		shifterColors[(i+distanceToMove)%colorsToMove] = originalPalette->colors[firstColorID + i];

	SDL_SetPaletteColors(currentPalette, shifterColors.data(), firstColorID, colorsToMove);
}

void SDLImageIndexed::adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask)
{
	const SDL_Palette * originalPalette = image->getPalette();

	// Note: here we skip first colors in the palette that are predefined in H3 images
	for(int i = 0; i < currentPalette->ncolors; i++)
	{
		if(i < std::numeric_limits<uint32_t>::digits && ((colorsToSkipMask >> i) & 1) == 1)
			continue;

		currentPalette->colors[i] = CSDL_Ext::toSDL(shifter.shiftColor(CSDL_Ext::fromSDL(originalPalette->colors[i])));
	}
}

SDLImageIndexed::SDLImageIndexed(const std::shared_ptr<SDLImageConst> & image, EImageBlitMode mode)
	:SDLImageBase::SDLImageBase(image, mode)
{
	auto originalPalette = image->getPalette();

	currentPalette = SDL_AllocPalette(originalPalette->ncolors);
	SDL_SetPaletteColors(currentPalette, originalPalette->colors, 0, originalPalette->ncolors);
}

SDLImageIndexed::~SDLImageIndexed()
{
	SDL_FreePalette(currentPalette);
}

void SDLImageIndexed::setSpecialPalette(const IImage::SpecialPalette & specialPalette, uint32_t colorsToSkipMask)
{
	size_t last = std::min<size_t>(specialPalette.size(), currentPalette->ncolors);

	for (size_t i = 0; i < last; ++i)
	{
		if(i < std::numeric_limits<uint32_t>::digits && ((colorsToSkipMask >> i) & 1) == 1)
			currentPalette->colors[i] = CSDL_Ext::toSDL(specialPalette[i]);
	}
}

SDLImageConst::~SDLImageConst()
{
	SDL_FreeSurface(surf);
	SDL_FreePalette(originalPalette);
}

SDLImageBase::SDLImageBase(const std::shared_ptr<SDLImageConst> & image, EImageBlitMode mode)
	:image(image)
	, alphaValue(SDL_ALPHA_OPAQUE)
	, blitMode(mode)
{}

void SDLImageRGB::draw(SDL_Surface * where, const Point & pos, const Rect * src) const
{
	image->draw(where, nullptr, pos, src, alphaValue, blitMode);
}

void SDLImageIndexed::draw(SDL_Surface * where, const Point & pos, const Rect * src) const
{
	image->draw(where, currentPalette, pos, src, alphaValue, blitMode);
}

void SDLImageBase::scaleFast(const Point & size)
{
	image = image->scaleFast(size);
}

void SDLImageBase::exportBitmap(const boost::filesystem::path & path) const
{
	image->exportBitmap(path);
}

bool SDLImageBase::isTransparent(const Point & coords) const
{
	return image->isTransparent(coords);
}

Point SDLImageBase::dimensions() const
{
	return image->dimensions();
}

void SDLImageBase::setAlpha(uint8_t value)
{
	alphaValue = value;
}

void SDLImageBase::setBlitMode(EImageBlitMode mode)
{
	blitMode = mode;
}

void SDLImageRGB::setSpecialPalette(const SpecialPalette & SpecialPalette, uint32_t colorsToSkipMask)
{}

void SDLImageRGB::playerColored(PlayerColor player)
{}

void SDLImageRGB::setFlagColor(PlayerColor player)
{}

void SDLImageRGB::shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove)
{}

void SDLImageRGB::adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask)
{}


