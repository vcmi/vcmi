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
#include "../render/Colors.h"
#include "../render/CBitmapHandler.h"
#include "../render/CDefFile.h"
#include "../render/Graphics.h"
#include "../xBRZ/xbrz.h"
#include "../gui/CGuiHandler.h"
#include "../render/IScreenHandler.h"

#ifdef VCMI_HTML5_BUILD
#include "html5/html5.h"
#endif

#include <tbb/parallel_for.h>
#include <SDL_surface.h>
#include <SDL_image.h>

class SDLImageLoader;

//First 8 colors in def palette used for transparency
static constexpr std::array<SDL_Color, 8> sourcePalette = {{
	{0,   255, 255, SDL_ALPHA_OPAQUE},
	{255, 150, 255, SDL_ALPHA_OPAQUE},
	{255, 100, 255, SDL_ALPHA_OPAQUE},
	{255, 50,  255, SDL_ALPHA_OPAQUE},
	{255, 0,   255, SDL_ALPHA_OPAQUE},
	{255, 255, 0,   SDL_ALPHA_OPAQUE},
	{180, 0,   255, SDL_ALPHA_OPAQUE},
	{0,   255, 0,   SDL_ALPHA_OPAQUE}
}};

static constexpr std::array<ColorRGBA, 8> targetPalette = {{
	{0, 0, 0, 0  }, // 0 - transparency                  ( used in most images )
	{0, 0, 0, 64 }, // 1 - shadow border                 ( used in battle, adventure map def's )
	{0, 0, 0, 64 }, // 2 - shadow border                 ( used in fog-of-war def's )
	{0, 0, 0, 128}, // 3 - shadow body                   ( used in fog-of-war def's )
	{0, 0, 0, 128}, // 4 - shadow body                   ( used in battle, adventure map def's )
	{0, 0, 0, 0  }, // 5 - selection / owner flag        ( used in battle, adventure map def's )
	{0, 0, 0, 128}, // 6 - shadow body   below selection ( used in battle def's )
	{0, 0, 0, 64 }  // 7 - shadow border below selection ( used in battle def's )
}};

static ui8 mixChannels(ui8 c1, ui8 c2, ui8 a1, ui8 a2)
{
	return c1*a1 / 256 + c2*a2*(255 - a1) / 256 / 256;
}

static ColorRGBA addColors(const ColorRGBA & base, const ColorRGBA & over)
{
	return ColorRGBA(
		mixChannels(over.r, base.r, over.a, base.a),
		mixChannels(over.g, base.g, over.a, base.a),
		mixChannels(over.b, base.b, over.a, base.a),
		static_cast<ui8>(over.a + base.a * (255 - over.a) / 256)
		);
}

static bool colorsSimilar (const SDL_Color & lhs, const SDL_Color & rhs)
{
	// it seems that H3 does not requires exact match to replace colors -> (255, 103, 255) gets interpreted as shadow
	// exact logic is not clear and requires extensive testing with image editing
	// potential reason is that H3 uses 16-bit color format (565 RGB bits), meaning that 3 least significant bits are lost in red and blue component
	static const int threshold = 8;

	int diffR = static_cast<int>(lhs.r) - rhs.r;
	int diffG = static_cast<int>(lhs.g) - rhs.g;
	int diffB = static_cast<int>(lhs.b) - rhs.b;
	int diffA = static_cast<int>(lhs.a) - rhs.a;

	return std::abs(diffR) < threshold && std::abs(diffG) < threshold && std::abs(diffB) < threshold && std::abs(diffA) < threshold;
}

int IImage::width() const
{
	return dimensions().x;
}

int IImage::height() const
{
	return dimensions().y;
}

SDLImageShared::SDLImageShared(const CDefFile * data, size_t frame, size_t group, int preScaleFactor)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr),
	preScaleFactor(preScaleFactor)
{
	SDLImageLoader loader(this);
	data->loadFrame(frame, group, loader);

	savePalette();
}

SDLImageShared::SDLImageShared(SDL_Surface * from, int preScaleFactor)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr),
	preScaleFactor(preScaleFactor)
{
	surf = from;
	if (surf == nullptr)
		return;

	savePalette();

	surf->refcount++;
	fullSize.x = surf->w;
	fullSize.y = surf->h;
}

SDLImageShared::SDLImageShared(const ImagePath & filename, int preScaleFactor)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr),
	preScaleFactor(preScaleFactor)
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

		optimizeSurface();
	}
}


void SDLImageShared::draw(SDL_Surface * where, SDL_Palette * palette, const Point & dest, const Rect * src, const ColorRGBA & colorMultiplier, uint8_t alpha, EImageBlitMode mode) const
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

	SDL_SetSurfaceColorMod(surf, colorMultiplier.r, colorMultiplier.g, colorMultiplier.b);
	SDL_SetSurfaceAlphaMod(surf, alpha);

	if (alpha != SDL_ALPHA_OPAQUE || (mode != EImageBlitMode::OPAQUE && surf->format->Amask != 0))
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_BLEND);
	else
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);

	if (palette && surf->format->palette)
		SDL_SetSurfacePalette(surf, palette);

	if(surf->format->palette && mode != EImageBlitMode::OPAQUE && mode != EImageBlitMode::COLORKEY)
	{
		CSDL_Ext::blit8bppAlphaTo24bpp(surf, sourceRect, where, destShift, alpha);
	}
	else
	{
		CSDL_Ext::blitSurface(surf, sourceRect, where, destShift);
	}

	if (surf->format->palette)
		SDL_SetSurfacePalette(surf, originalPalette);
}

void SDLImageShared::optimizeSurface()
{
	if (!surf)
		return;

	int left = surf->w;
	int top = surf->h;
	int right = 0;
	int bottom = 0;

	// locate fully-transparent area around image
	// H3 hadles this on format level, but mods or images scaled in runtime do not
	if (surf->format->palette)
	{
		for (int y = 0; y < surf->h; ++y)
		{
			const uint8_t * row = static_cast<uint8_t *>(surf->pixels) + y * surf->pitch;
			for (int x = 0; x < surf->w; ++x)
			{
				if (row[x] != 0)
				{
					// opaque or can be opaque (e.g. disabled shadow)
					top = std::min(top, y);
					left = std::min(left, x);
					right = std::max(right, x);
					bottom = std::max(bottom, y);
				}
			}
		}
	}
	else
	{
		for (int y = 0; y < surf->h; ++y)
		{
			for (int x = 0; x < surf->w; ++x)
			{
				ColorRGBA color;
				SDL_GetRGBA(CSDL_Ext::getPixel(surf, x, y), surf->format, &color.r, &color.g, &color.b, &color.a);

				if (color.a != SDL_ALPHA_TRANSPARENT)
				{
					 // opaque
					top = std::min(top, y);
					left = std::min(left, x);
					right = std::max(right, x);
					bottom = std::max(bottom, y);
				}
			}
		}
	}

	if (left == surf->w)
	{
		// empty image - simply delete it
		SDL_FreeSurface(surf);
		surf = nullptr;
		return;
	}

	if (left != 0 || top != 0 || right != surf->w - 1 || bottom != surf->h - 1)
	{
		// non-zero border found
		Rect newDimensions(left, top, right - left + 1, bottom - top + 1);
		SDL_Rect rectSDL = CSDL_Ext::toSDL(newDimensions);
		auto newSurface = CSDL_Ext::newSurface(newDimensions.dimensions(), surf);
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);
		SDL_BlitSurface(surf, &rectSDL, newSurface, nullptr);

		if (SDL_HasColorKey(surf))
		{
			uint32_t colorKey;
			SDL_GetColorKey(surf, &colorKey);
			SDL_SetColorKey(newSurface, SDL_TRUE, colorKey);
		}

		SDL_FreeSurface(surf);
		surf = newSurface;

		margins.x += left;
		margins.y += top;
	}
}

std::shared_ptr<const ISharedImage> SDLImageShared::scaleInteger(int factor, SDL_Palette * palette, EImageBlitMode mode) const
{
	if (factor <= 0)
		throw std::runtime_error("Unable to scale by integer value of " + std::to_string(factor));

	if (!surf)
		return shared_from_this();

	if (palette && surf->format->palette)
		SDL_SetSurfacePalette(surf, palette);

	SDL_Surface * scaled = nullptr;
	if(preScaleFactor == factor)
		return shared_from_this();
	else if(preScaleFactor == 1)
	{
		// dump heuristics to differentiate tileable UI elements from map object / combat assets
		if (mode == EImageBlitMode::OPAQUE || mode == EImageBlitMode::COLORKEY || mode == EImageBlitMode::SIMPLE)
			scaled = CSDL_Ext::scaleSurfaceIntegerFactor(surf, factor, EScalingAlgorithm::XBRZ_OPAQUE);
		else
			scaled = CSDL_Ext::scaleSurfaceIntegerFactor(surf, factor, EScalingAlgorithm::XBRZ_ALPHA);
	}
	else
		scaled = CSDL_Ext::scaleSurface(surf, (int) round((float)surf->w * factor / preScaleFactor), (int) round((float)surf->h * factor / preScaleFactor));

	auto ret = std::make_shared<SDLImageShared>(scaled, preScaleFactor);

	ret->fullSize.x = fullSize.x * factor;
	ret->fullSize.y = fullSize.y * factor;

	ret->margins.x = (int) round((float)margins.x * factor / preScaleFactor);
	ret->margins.y = (int) round((float)margins.y * factor / preScaleFactor);
	ret->optimizeSurface();

	// erase our own reference
	SDL_FreeSurface(scaled);

	if (surf->format->palette)
		SDL_SetSurfacePalette(surf, originalPalette);

	return ret;
}

std::shared_ptr<const ISharedImage> SDLImageShared::scaleTo(const Point & size, SDL_Palette * palette) const
{
	float scaleX = static_cast<float>(size.x) / fullSize.x;
	float scaleY = static_cast<float>(size.y) / fullSize.y;

	if (palette && surf->format->palette)
		SDL_SetSurfacePalette(surf, palette);

	auto scaled = CSDL_Ext::scaleSurface(surf, (int)(surf->w * scaleX), (int)(surf->h * scaleY));

	if (scaled->format && scaled->format->palette) // fix color keying, because SDL loses it at this point
		CSDL_Ext::setColorKey(scaled, scaled->format->palette->colors[0]);
	else if(scaled->format && scaled->format->Amask)
		SDL_SetSurfaceBlendMode(scaled, SDL_BLENDMODE_BLEND);//just in case
	else
		CSDL_Ext::setDefaultColorKey(scaled);//just in case

	auto ret = std::make_shared<SDLImageShared>(scaled, preScaleFactor);

	ret->fullSize.x = (int) round((float)fullSize.x * scaleX);
	ret->fullSize.y = (int) round((float)fullSize.y * scaleY);

	ret->margins.x = (int) round((float)margins.x * scaleX);
	ret->margins.y = (int) round((float)margins.y * scaleY);

	// erase our own reference
	SDL_FreeSurface(scaled);

	if (surf->format->palette)
		SDL_SetSurfacePalette(surf, originalPalette);

	return ret;
}

void SDLImageShared::exportBitmap(const boost::filesystem::path& path, SDL_Palette * palette) const
{
	if (!surf)
		return;

	if (palette && surf->format->palette)
		SDL_SetSurfacePalette(surf, palette);
#ifdef VCMI_HTML5_BUILD
	html5::savePng(surf, path.string().c_str());
#else
	IMG_SavePNG(surf, path.string().c_str());
#endif
	if (palette && surf->format->palette)
		SDL_SetSurfacePalette(surf, originalPalette);
}

void SDLImageIndexed::playerColored(PlayerColor player)
{
	graphics->setPlayerPalette(currentPalette, player);
}

bool SDLImageShared::isTransparent(const Point & coords) const
{
	if (surf)
		return CSDL_Ext::isTransparent(surf, coords.x - margins.x, coords.y	- margins.y);
	else
		return true;
}

Rect SDLImageShared::contentRect() const
{
	auto tmpMargins = margins / preScaleFactor;
	auto tmpSize = Point(surf->w, surf->h) / preScaleFactor;
	return Rect(tmpMargins, tmpSize);
}

Point SDLImageShared::dimensions() const
{
	return fullSize / preScaleFactor;
}

std::shared_ptr<IImage> SDLImageShared::createImageReference(EImageBlitMode mode) const
{
	if (surf && surf->format->palette)
		return std::make_shared<SDLImageIndexed>(shared_from_this(), originalPalette, mode);
	else
		return std::make_shared<SDLImageRGB>(shared_from_this(), mode);
}

std::shared_ptr<const ISharedImage> SDLImageShared::horizontalFlip() const
{
	if (!surf)
		return shared_from_this();

	SDL_Surface * flipped = CSDL_Ext::horizontalFlip(surf);
	auto ret = std::make_shared<SDLImageShared>(flipped, preScaleFactor);
	ret->fullSize = fullSize;
	ret->margins.x = margins.x;
	ret->margins.y = fullSize.y - surf->h - margins.y;
	ret->fullSize = fullSize;

	return ret;
}

std::shared_ptr<const ISharedImage> SDLImageShared::verticalFlip() const
{
	if (!surf)
		return shared_from_this();

	SDL_Surface * flipped = CSDL_Ext::verticalFlip(surf);
	auto ret = std::make_shared<SDLImageShared>(flipped, preScaleFactor);
	ret->fullSize = fullSize;
	ret->margins.x = fullSize.x - surf->w - margins.x;
	ret->margins.y = margins.y;
	ret->fullSize = fullSize;

	return ret;
}

// Keep the original palette, in order to do color switching operation
void SDLImageShared::savePalette()
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
	std::vector<SDL_Color> shifterColors(colorsToMove);

	for(uint32_t i=0; i<colorsToMove; ++i)
		shifterColors[(i+distanceToMove)%colorsToMove] = originalPalette->colors[firstColorID + i];

	SDL_SetPaletteColors(currentPalette, shifterColors.data(), firstColorID, colorsToMove);
}

void SDLImageIndexed::adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask)
{
	// If shadow is enabled, following colors must be skipped unconditionally
	if (blitMode == EImageBlitMode::WITH_SHADOW || blitMode == EImageBlitMode::WITH_SHADOW_AND_OVERLAY)
		colorsToSkipMask |= (1 << 0) + (1 << 1) + (1 << 4);

	// Note: here we skip first colors in the palette that are predefined in H3 images
	for(int i = 0; i < currentPalette->ncolors; i++)
	{
		if (i < std::size(sourcePalette) && colorsSimilar(sourcePalette[i], originalPalette->colors[i]))
			continue;

		if(i < std::numeric_limits<uint32_t>::digits && ((colorsToSkipMask >> i) & 1) == 1)
			continue;

		currentPalette->colors[i] = CSDL_Ext::toSDL(shifter.shiftColor(CSDL_Ext::fromSDL(originalPalette->colors[i])));
	}
}

SDLImageIndexed::SDLImageIndexed(const std::shared_ptr<const ISharedImage> & image, SDL_Palette * originalPalette, EImageBlitMode mode)
	:SDLImageBase::SDLImageBase(image, mode)
	,originalPalette(originalPalette)
{
	currentPalette = SDL_AllocPalette(originalPalette->ncolors);
	SDL_SetPaletteColors(currentPalette, originalPalette->colors, 0, originalPalette->ncolors);

	preparePalette();
}

SDLImageIndexed::~SDLImageIndexed()
{
	SDL_FreePalette(currentPalette);
}

void SDLImageIndexed::setShadowTransparency(float factor)
{
	ColorRGBA shadow50(0, 0, 0, 128 * factor);
	ColorRGBA shadow25(0, 0, 0,  64 * factor);

	std::array<SDL_Color, 5> colorsSDL = {
		originalPalette->colors[0],
		originalPalette->colors[1],
		originalPalette->colors[2],
		originalPalette->colors[3],
		originalPalette->colors[4]
	};

	// seems to be used unconditionally
	colorsSDL[0] = CSDL_Ext::toSDL(Colors::TRANSPARENCY);
	colorsSDL[1] = CSDL_Ext::toSDL(shadow25);
	colorsSDL[4] = CSDL_Ext::toSDL(shadow50);

	// seems to be used only if color matches
	if (colorsSimilar(originalPalette->colors[2], sourcePalette[2]))
		colorsSDL[2] = CSDL_Ext::toSDL(shadow25);

	if (colorsSimilar(originalPalette->colors[3], sourcePalette[3]))
		colorsSDL[3] = CSDL_Ext::toSDL(shadow50);

	SDL_SetPaletteColors(currentPalette, colorsSDL.data(), 0, colorsSDL.size());
}

void SDLImageIndexed::setOverlayColor(const ColorRGBA & color)
{
	currentPalette->colors[5] = CSDL_Ext::toSDL(addColors(targetPalette[5], color));

	for (int i : {6,7})
	{
		if (colorsSimilar(originalPalette->colors[i], sourcePalette[i]))
			currentPalette->colors[i] = CSDL_Ext::toSDL(addColors(targetPalette[i], color));
	}
}

void SDLImageIndexed::preparePalette()
{
	switch(blitMode)
	{
		case EImageBlitMode::ONLY_SHADOW:
		case EImageBlitMode::ONLY_OVERLAY:
			adjustPalette(ColorFilter::genAlphaShifter(0), 0);
			break;
	}

	switch(blitMode)
	{
		case EImageBlitMode::SIMPLE:
		case EImageBlitMode::WITH_SHADOW:
		case EImageBlitMode::ONLY_SHADOW:
		case EImageBlitMode::WITH_SHADOW_AND_OVERLAY:
			setShadowTransparency(1.0);
			break;
		case EImageBlitMode::ONLY_BODY:
		case EImageBlitMode::ONLY_BODY_IGNORE_OVERLAY:
		case EImageBlitMode::ONLY_OVERLAY:
			setShadowTransparency(0.0);
			break;
	}

	switch(blitMode)
	{
		case EImageBlitMode::ONLY_OVERLAY:
		case EImageBlitMode::WITH_SHADOW_AND_OVERLAY:
			setOverlayColor(Colors::WHITE_TRUE);
			break;
		case EImageBlitMode::ONLY_SHADOW:
		case EImageBlitMode::ONLY_BODY:
			setOverlayColor(Colors::TRANSPARENCY);
			break;
	}
}

SDLImageShared::~SDLImageShared()
{
	SDL_FreeSurface(surf);
	SDL_FreePalette(originalPalette);
}

SDLImageBase::SDLImageBase(const std::shared_ptr<const ISharedImage> & image, EImageBlitMode mode)
	:image(image)
	, alphaValue(SDL_ALPHA_OPAQUE)
	, blitMode(mode)
{}

std::shared_ptr<const ISharedImage> SDLImageBase::getSharedImage() const
{
	return image;
}

void SDLImageRGB::draw(SDL_Surface * where, const Point & pos, const Rect * src) const
{
	image->draw(where, nullptr, pos, src, Colors::WHITE_TRUE, alphaValue, blitMode);
}

void SDLImageIndexed::draw(SDL_Surface * where, const Point & pos, const Rect * src) const
{
	image->draw(where, currentPalette, pos, src, Colors::WHITE_TRUE, alphaValue, blitMode);
}

void SDLImageIndexed::exportBitmap(const boost::filesystem::path & path) const
{
	image->exportBitmap(path, currentPalette);
}

void SDLImageIndexed::scaleTo(const Point & size)
{
	image = image->scaleTo(size, currentPalette);
}

void SDLImageRGB::scaleTo(const Point & size)
{
	image = image->scaleTo(size, nullptr);
}

void SDLImageIndexed::scaleInteger(int factor)
{
	image = image->scaleInteger(factor, currentPalette, blitMode);
}

void SDLImageRGB::scaleInteger(int factor)
{
	image = image->scaleInteger(factor, nullptr, blitMode);
}

void SDLImageRGB::exportBitmap(const boost::filesystem::path & path) const
{
	image->exportBitmap(path, nullptr);
}

bool SDLImageBase::isTransparent(const Point & coords) const
{
	return image->isTransparent(coords);
}

Rect SDLImageBase::contentRect() const
{
	return image->contentRect();
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

void SDLImageRGB::setOverlayColor(const ColorRGBA & color)
{}

void SDLImageRGB::playerColored(PlayerColor player)
{}

void SDLImageRGB::shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove)
{}

void SDLImageRGB::adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask)
{}


