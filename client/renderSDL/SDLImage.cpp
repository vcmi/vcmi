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
#include "SDLImageScaler.h"
#include "SDL_Extensions.h"

#include "../render/ColorFilter.h"
#include "../render/CBitmapHandler.h"
#include "../render/CDefFile.h"
#include "../GameEngine.h"
#include "../render/IScreenHandler.h"

#include "../../lib/AsyncRunner.h"
#include "../../lib/CConfigHandler.h"

#include <tbb/parallel_for.h>

#include <SDL_image.h>
#include <SDL_surface.h>
#include <SDL_version.h>

class SDLImageLoader;

int IImage::width() const
{
	return dimensions().x;
}

int IImage::height() const
{
	return dimensions().y;
}

SDLImageShared::SDLImageShared(const CDefFile * data, size_t frame, size_t group)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr)
{
	SDLImageLoader loader(this);
	data->loadFrame(frame, group, loader);

	savePalette();
}

SDLImageShared::SDLImageShared(SDL_Surface * from)
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

SDLImageShared::SDLImageShared(const ImagePath & filename, bool optimizeImage)
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

		if(optimizeImage)
			optimizeSurface();
	}
}

void SDLImageShared::scaledDraw(SDL_Surface * where, SDL_Palette * palette, const Point & scaleTo, const Point & dest, const Rect * src, const ColorRGBA & colorMultiplier, uint8_t alpha, EImageBlitMode mode) const
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	if (!surf)
		return;

	Rect sourceRect(0, 0, surf->w, surf->h);
	Point destShift(0, 0);
	Point destScale = Point(surf->w, surf->h) * scaleTo / dimensions();
	Point marginsScaled = margins * scaleTo / dimensions();

	if(src)
	{
		Rect srcUnscaled(Point(src->topLeft() * dimensions() / scaleTo), Point(src->dimensions() * dimensions() / scaleTo));

		if(srcUnscaled.x < margins.x)
			destShift.x += marginsScaled.x - src->x;

		if(srcUnscaled.y < margins.y)
			destShift.y += marginsScaled.y - src->y;

		sourceRect = Rect(srcUnscaled).intersect(Rect(margins.x, margins.y, surf->w, surf->h));

		destScale.x = std::min(destScale.x, sourceRect.w * scaleTo.x / dimensions().x);
		destScale.y = std::min(destScale.y, sourceRect.h * scaleTo.y / dimensions().y);

		sourceRect -= margins;
	}
	else
		destShift = marginsScaled;

	destShift += dest;

	SDL_SetSurfaceColorMod(surf, colorMultiplier.r, colorMultiplier.g, colorMultiplier.b);
	SDL_SetSurfaceAlphaMod(surf, alpha);

	if (alpha != SDL_ALPHA_OPAQUE || (mode != EImageBlitMode::OPAQUE && surf->format->Amask != 0))
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_BLEND);
	else
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);

	if (palette && surf->format->palette)
		SDL_SetSurfacePalette(surf, palette);

	SDL_Rect srcRect = CSDL_Ext::toSDL(sourceRect);
	SDL_Rect dstRect = CSDL_Ext::toSDL(Rect(destShift, destScale));

	if (sourceRect.dimensions() * scaleTo / dimensions() != destScale)
		logGlobal->info("???");

	SDL_Surface * tempSurface = SDL_ConvertSurface(surf, where->format, 0);
	int result = SDL_BlitScaled(tempSurface, &srcRect, where, &dstRect);

	SDL_FreeSurface(tempSurface);
	if (result != 0)
		logGlobal->error("SDL_BlitScaled failed! %s", SDL_GetError());

	if (surf->format->palette)
		SDL_SetSurfacePalette(surf, originalPalette);
}

void SDLImageShared::draw(SDL_Surface * where, SDL_Palette * palette, const Point & dest, const Rect * src, const ColorRGBA & colorMultiplier, uint8_t alpha, EImageBlitMode mode) const
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

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
	assert(upscalingInProgress == false);
	if (!surf)
		return;

	SDLImageOptimizer optimizer(surf, Rect(margins, fullSize));

	optimizer.optimizeSurface(surf);
	SDL_FreeSurface(surf);

	surf = optimizer.acquireResultSurface();
	margins = optimizer.getResultDimensions().topLeft();
	fullSize = optimizer.getResultDimensions().dimensions();
}

std::shared_ptr<const ISharedImage> SDLImageShared::scaleInteger(int factor, SDL_Palette * palette, EImageBlitMode mode) const
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	if (factor <= 0)
		throw std::runtime_error("Unable to scale by integer value of " + std::to_string(factor));

	if (!surf)
		return shared_from_this();

	if (palette && surf->format->palette)
		SDL_SetSurfacePalette(surf, palette);

	// simple heuristics to differentiate tileable UI elements from map object / combat assets
	EScalingAlgorithm algorithm;
	if (mode == EImageBlitMode::OPAQUE || mode == EImageBlitMode::COLORKEY || mode == EImageBlitMode::SIMPLE)
		algorithm = EScalingAlgorithm::XBRZ_OPAQUE;
	else
		algorithm = EScalingAlgorithm::XBRZ_ALPHA;

	auto result = SDLImageShared::createScaled(this, factor, algorithm);

	if (surf->format->palette)
		SDL_SetSurfacePalette(surf, originalPalette);

	return result;
}

std::shared_ptr<SDLImageShared> SDLImageShared::createScaled(const SDLImageShared * from, int integerScaleFactor, EScalingAlgorithm algorithm)
{
	auto self = std::make_shared<SDLImageShared>(nullptr);
	self->upscalingInProgress = true;

	auto scaler = std::make_shared<SDLImageScaler>(from->surf, Rect(from->margins, from->fullSize), true);

	const auto & scalingTask = [self, algorithm, scaler]()
	{
		scaler->scaleSurfaceIntegerFactor(ENGINE->screenHandler().getScalingFactor(), algorithm);
		self->surf = scaler->acquireResultSurface();
		self->fullSize = scaler->getResultDimensions().dimensions();
		self->margins = scaler->getResultDimensions().topLeft();
		self->upscalingInProgress = false;
	};

	if(settings["video"]["asyncUpscaling"].Bool() && from->getAsyncUpscale())
		ENGINE->async().run(scalingTask);
	else
		scalingTask();

	return self;
}

bool SDLImageShared::isLoading() const
{
	return upscalingInProgress;
}

void SDLImageShared::setAsyncUpscale(bool on)
{
	asyncUpscale = on;
}

bool SDLImageShared::getAsyncUpscale() const
{
	return asyncUpscale;
}

std::shared_ptr<const ISharedImage> SDLImageShared::scaleTo(const Point & size, SDL_Palette * palette) const
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	if (palette && surf->format->palette)
		SDL_SetSurfacePalette(surf, palette);

	SDLImageScaler scaler(surf, Rect(margins, fullSize), true);

	scaler.scaleSurface(size, EScalingAlgorithm::XBRZ_ALPHA);

	auto scaled = scaler.acquireResultSurface();

	if (scaled->format && scaled->format->palette) // fix color keying, because SDL loses it at this point
		CSDL_Ext::setColorKey(scaled, scaled->format->palette->colors[0]);
	else if(scaled->format && scaled->format->Amask)
		SDL_SetSurfaceBlendMode(scaled, SDL_BLENDMODE_BLEND);//just in case
	else
		CSDL_Ext::setDefaultColorKey(scaled);//just in case

	auto ret = std::make_shared<SDLImageShared>(scaled);
	ret->fullSize = scaler.getResultDimensions().dimensions();
	ret->margins = scaler.getResultDimensions().topLeft();

	// erase our own reference
	SDL_FreeSurface(scaled);

	if (surf->format->palette)
		SDL_SetSurfacePalette(surf, originalPalette);

	return ret;
}

void SDLImageShared::exportBitmap(const boost::filesystem::path& path, SDL_Palette * palette) const
{
	auto directory = path;
	directory.remove_filename();
	boost::filesystem::create_directories(directory);

	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	if (!surf)
		return;

	if (palette && surf->format->palette)
		SDL_SetSurfacePalette(surf, palette);
	IMG_SavePNG(surf, path.string().c_str());
	if (palette && surf->format->palette)
		SDL_SetSurfacePalette(surf, originalPalette);
}

bool SDLImageShared::isTransparent(const Point & coords) const
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	if (!surf)
		return true;

	Point test = coords - margins;

	if (test.x < 0 || test.y < 0 || test.x >= surf->w || test.y >= surf->h)
		return true;

	SDL_Color color;
	SDL_GetRGBA(CSDL_Ext::getPixel(surf, test.x, test.y), surf->format, &color.r, &color.g, &color.b, &color.a);

	bool pixelTransparent = color.a < 128;
	bool pixelCyan = (color.r == 0 && color.g == 255 && color.b == 255);

	return pixelTransparent || pixelCyan;
}

Rect SDLImageShared::contentRect() const
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	if (!surf)
		return Rect();

	return Rect(margins, Point(surf->w, surf->h));
}

const SDL_Palette * SDLImageShared::getPalette() const
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	if (!surf)
		return nullptr;
	return surf->format->palette;
}

Point SDLImageShared::dimensions() const
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	return fullSize;
}

std::shared_ptr<const ISharedImage> SDLImageShared::horizontalFlip() const
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	if (!surf)
		return shared_from_this();

	SDL_Surface * flipped = CSDL_Ext::horizontalFlip(surf);
	auto ret = std::make_shared<SDLImageShared>(flipped);
	ret->fullSize = fullSize;
	ret->margins.x = margins.x;
	ret->margins.y = fullSize.y - surf->h - margins.y;
	ret->fullSize = fullSize;

	// erase our own reference
	SDL_FreeSurface(flipped);

	return ret;
}

std::shared_ptr<const ISharedImage> SDLImageShared::verticalFlip() const
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	if (!surf)
		return shared_from_this();

	SDL_Surface * flipped = CSDL_Ext::verticalFlip(surf);
	auto ret = std::make_shared<SDLImageShared>(flipped);
	ret->fullSize = fullSize;
	ret->margins.x = fullSize.x - surf->w - margins.x;
	ret->margins.y = margins.y;
	ret->fullSize = fullSize;

	// erase our own reference
	SDL_FreeSurface(flipped);

	return ret;
}

std::shared_ptr<SDLImageShared> SDLImageShared::drawShadow(bool doSheer) const
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	if (!surf)
		return nullptr;

	SDL_Surface * shadow = CSDL_Ext::drawShadow(surf, doSheer);
	auto ret = std::make_shared<SDLImageShared>(shadow);
	ret->fullSize = fullSize;
	ret->margins.x = margins.x;
	ret->margins.y = margins.y;
	ret->optimizeSurface();

	// erase our own reference
	SDL_FreeSurface(shadow);

	return ret;
}

std::shared_ptr<SDLImageShared> SDLImageShared::drawOutline(const ColorRGBA & color, int thickness) const
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	if (!surf)
		return nullptr;

	SDL_Color sdlColor = { color.r, color.g, color.b, color.a };
	SDL_Surface * outline = CSDL_Ext::drawOutline(surf, sdlColor, thickness);
	auto ret = std::make_shared<SDLImageShared>(outline);
	ret->fullSize = fullSize;
	ret->margins.x = margins.x;
	ret->margins.y = margins.y;
	ret->optimizeSurface();

	// erase our own reference
	SDL_FreeSurface(outline);

	return ret;
}

void SDLImageShared::setMargins(const Point & newMargins)
{
	margins = newMargins;
}

void SDLImageShared::setFullSize(const Point & newSize)
{
	fullSize = newSize;
}

// Keep the original palette, in order to do color switching operation
void SDLImageShared::savePalette()
{
	if(upscalingInProgress)
		throw std::runtime_error("Attempt to access images that is still being loaded!");

	// For some images that don't have palette, skip this
	if(surf->format->palette == nullptr)
		return;

	if(originalPalette == nullptr)
		originalPalette = SDL_AllocPalette(surf->format->palette->ncolors);

	SDL_SetPaletteColors(originalPalette, surf->format->palette->colors, 0, surf->format->palette->ncolors);
}

SDLImageShared::~SDLImageShared()
{
	SDL_FreeSurface(surf);
	if (originalPalette)
		SDL_FreePalette(originalPalette);
}
