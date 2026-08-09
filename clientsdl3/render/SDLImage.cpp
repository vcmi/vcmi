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
#include "GpuResources.h"
#include "Profiler.h"
#include "RenderHandler.h"

#include "SDLImageLoader.h"
#include "SDLImageScaler.h"
#include "SDL_Extensions.h"

#include "render/ColorFilter.h"
#include "render/CBitmapHandler.h"
#include "render/CDefFile.h"
#include "CMT.h"
#include "GameEngine.h"
#include "IScreenHandler.h"

#include "lib/AsyncRunner.h"
#include "lib/CConfigHandler.h"

#include <tbb/parallel_for.h>

#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_version.h>

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

	if (alpha != SDL_ALPHA_OPAQUE || (mode != EImageBlitMode::OPAQUE && CSDL_Ext::getFormat(surf)->Amask != 0))
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_BLEND);
	else
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);

	if (palette && CSDL_Ext::getPalette(surf))
		SDL_SetSurfacePalette(surf, palette);

	SDL_Rect srcRect = CSDL_Ext::toSDL(sourceRect);
	SDL_Rect dstRect = CSDL_Ext::toSDL(Rect(destShift, destScale));

	if (sourceRect.dimensions() * scaleTo / dimensions() != destScale)
		logGlobal->info("???");

	SDL_Surface * tempSurface = SDL_ConvertSurface(surf, where->format);
	bool result = SDL_BlitSurfaceScaled(tempSurface, &srcRect, where, &dstRect, SDL_SCALEMODE_NEAREST);

	SDL_DestroySurface(tempSurface);
	if (!result)
		logGlobal->error("SDL_BlitSurfaceScaled failed! %s", SDL_GetError());

	if (CSDL_Ext::getPalette(surf))
		SDL_SetSurfacePalette(surf, originalPalette);
}

void SDLImageShared::dropTexture() const
{
	// destroying the renderer already destroyed everything it owned, so a stale
	// generation means the pointer must be dropped rather than freed
	if(texture && textureGeneration == GpuResources::get().generation())
		GpuResources::get().destroyTextureDeferred(texture);

	texture = nullptr;
}

SDL_Texture * SDLImageShared::getTexture(SDL_Palette * palette) const
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	if(upscalingInProgress || surf == nullptr || renderer == nullptr)
		return nullptr;

	// an upscaled image is plain RGBA - the palette never reaches its texture, so keying the
	// cache on it would rebuild the texture for every instance that shares the image
	SDL_Palette * effectivePalette = CSDL_Ext::getPalette(surf) ? palette : nullptr;

	if(texture && textureGeneration == GpuResources::get().generation() && texturePalette == effectivePalette)
		return texture;

	dropTexture();

	VCMI_PROFILE_N("DIAG: image texture (re)created");
	VCMI_PROFILE_VALUE(static_cast<uint64_t>(surf->w) * surf->h);

	if(effectivePalette)
		SDL_SetSurfacePalette(surf, palette);

	texture = SDL_CreateTextureFromSurface(renderer, surf);

	if(CSDL_Ext::getPalette(surf))
		SDL_SetSurfacePalette(surf, originalPalette);

	if(texture == nullptr)
		logGlobal->error("Failed to create texture from image! %s", SDL_GetError());

	texturePalette = effectivePalette;
	textureGeneration = GpuResources::get().generation();

	return texture;
}

bool SDLImageShared::scaledDrawTexture(SDL_Renderer * renderer, SDL_Palette * palette, const Point & scaleTo, const Point & dest, const Rect * src, const ColorRGBA & colorMultiplier, uint8_t alpha, EImageBlitMode mode, const ImageFlip & flip) const
{
	if(upscalingInProgress || !surf)
		return false;

	SDL_Texture * source = getTexture(palette);
	if(!source)
		return false;

	// same geometry as scaledDraw(), the GPU just does the stretching for us
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

	// mirroring moves the cropped-away margin to the opposite side. Only reached without a source
	// rectangle, which is what ScalableImageShared asks to mirror.
	if(flip.x)
		destShift.x = scaleTo.x - destScale.x - destShift.x;

	if(flip.y)
		destShift.y = scaleTo.y - destScale.y - destShift.y;

	destShift += dest;

	if(sourceRect.w <= 0 || sourceRect.h <= 0 || destScale.x <= 0 || destScale.y <= 0)
		return true;

	SDL_SetTextureColorMod(source, colorMultiplier.r, colorMultiplier.g, colorMultiplier.b);
	SDL_SetTextureAlphaMod(source, alpha);

	// Sprites must not inherit the renderer-wide scale quality: a smoothed stand-in would
	// visibly differ from the nearest-scaled xBRZ image that replaces it.
	SDL_SetTextureScaleMode(source, SDL_SCALEMODE_NEAREST);

	// Unlike the surface path this cannot test the alpha mask: SDL_CreateTextureFromSurface turns
	// a paletted surface's color key into real alpha, so only a truly opaque image may skip blending
	if(alpha != SDL_ALPHA_OPAQUE || mode != EImageBlitMode::OPAQUE)
		SDL_SetTextureBlendMode(source, SDL_BLENDMODE_BLEND);
	else
		SDL_SetTextureBlendMode(source, SDL_BLENDMODE_NONE);

	SDL_FRect sdlSource = CSDL_Ext::toSDLFloat(sourceRect);
	SDL_FRect sdlTarget = CSDL_Ext::toSDLFloat(Rect(destShift, destScale));

	if(flip.any())
	{
		const auto mode = static_cast<SDL_FlipMode>((flip.x ? SDL_FLIP_HORIZONTAL : 0) | (flip.y ? SDL_FLIP_VERTICAL : 0));
		SDL_RenderTextureRotated(renderer, source, &sdlSource, &sdlTarget, 0.0, nullptr, mode);
	}
	else
		SDL_RenderTexture(renderer, source, &sdlSource, &sdlTarget);

	return true;
}

bool SDLImageShared::drawTexture(SDL_Renderer * renderer, SDL_Palette * palette, const Point & dest, const Rect * src, const ColorRGBA & colorMultiplier, uint8_t alpha, EImageBlitMode mode, const ImageFlip & flip) const
{
	// drawing at native size is the scaled path with a scale of one, and the geometry
	// there reduces exactly to the unscaled case
	return scaledDrawTexture(renderer, palette, dimensions(), dest, src, colorMultiplier, alpha, mode, flip);
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

	if (alpha != SDL_ALPHA_OPAQUE || (mode != EImageBlitMode::OPAQUE && CSDL_Ext::getFormat(surf)->Amask != 0))
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_BLEND);
	else
		SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);

	if (palette && CSDL_Ext::getPalette(surf))
		SDL_SetSurfacePalette(surf, palette);

	if(CSDL_Ext::getPalette(surf) && mode != EImageBlitMode::OPAQUE && mode != EImageBlitMode::COLORKEY)
	{
		CSDL_Ext::blit8bppAlphaTo24bpp(surf, sourceRect, where, destShift, alpha);
	}
	else
	{
		CSDL_Ext::blitSurface(surf, sourceRect, where, destShift);
	}

	if (CSDL_Ext::getPalette(surf))
		SDL_SetSurfacePalette(surf, originalPalette);
}

void SDLImageShared::optimizeSurface()
{
	assert(upscalingInProgress == false);
	if (!surf)
		return;

	SDLImageOptimizer optimizer(surf, Rect(margins, fullSize));

	optimizer.optimizeSurface(surf);
	SDL_DestroySurface(surf);

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

	if (palette && CSDL_Ext::getPalette(surf))
		SDL_SetSurfacePalette(surf, palette);

	// simple heuristics to differentiate tileable UI elements from map object / combat assets
	EScalingAlgorithm algorithm;
	if (mode == EImageBlitMode::OPAQUE || mode == EImageBlitMode::COLORKEY || mode == EImageBlitMode::SIMPLE)
		algorithm = EScalingAlgorithm::XBRZ_OPAQUE;
	else
		algorithm = EScalingAlgorithm::XBRZ_ALPHA;

	auto result = SDLImageShared::createScaled(this, factor, algorithm);

	if (CSDL_Ext::getPalette(surf))
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

size_t SDLImageShared::bytesUsed() const
{
	if(surf == nullptr)
		return sizeof(SDLImageShared);

	// a surface can be shared between several images, so split its cost between the holders
	size_t pixelBytes = static_cast<size_t>(surf->h) * surf->pitch / surf->refcount;

	size_t paletteBytes = 0;
	if(SDL_Palette * palette = CSDL_Ext::getPalette(surf))
		paletteBytes = static_cast<size_t>(palette->ncolors) * sizeof(SDL_Color);

	return pixelBytes + paletteBytes + sizeof(SDLImageShared);
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

	if (palette && CSDL_Ext::getPalette(surf))
		SDL_SetSurfacePalette(surf, palette);

	SDLImageScaler scaler(surf, Rect(margins, fullSize), true);

	scaler.scaleSurface(size, EScalingAlgorithm::XBRZ_ALPHA);

	auto scaled = scaler.acquireResultSurface();

	if (scaled->format && CSDL_Ext::getPalette(scaled)) // fix color keying, because SDL loses it at this point
		CSDL_Ext::setColorKey(scaled, CSDL_Ext::getPalette(scaled)->colors[0]);
	else if(scaled->format && CSDL_Ext::getFormat(scaled)->Amask)
		SDL_SetSurfaceBlendMode(scaled, SDL_BLENDMODE_BLEND);//just in case
	else
		CSDL_Ext::setDefaultColorKey(scaled);//just in case

	auto ret = std::make_shared<SDLImageShared>(scaled);
	ret->fullSize = scaler.getResultDimensions().dimensions();
	ret->margins = scaler.getResultDimensions().topLeft();

	// erase our own reference
	SDL_DestroySurface(scaled);

	if (CSDL_Ext::getPalette(surf))
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

	if (palette && CSDL_Ext::getPalette(surf))
		SDL_SetSurfacePalette(surf, palette);
	IMG_SavePNG(surf, path.string().c_str());
	if (palette && CSDL_Ext::getPalette(surf))
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

	SDL_Color color = CSDL_Ext::getColor(surf, CSDL_Ext::getPixel(surf, test.x, test.y));

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
	return CSDL_Ext::getPalette(surf);
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
	SDL_DestroySurface(flipped);

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
	SDL_DestroySurface(flipped);

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
	SDL_DestroySurface(shadow);

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
	SDL_DestroySurface(outline);

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
	if(CSDL_Ext::getPalette(surf) == nullptr)
		return;

	if(originalPalette == nullptr)
		originalPalette = SDL_CreatePalette(CSDL_Ext::getPalette(surf)->ncolors);

	SDL_SetPaletteColors(originalPalette, CSDL_Ext::getPalette(surf)->colors, 0, CSDL_Ext::getPalette(surf)->ncolors);
}

SDLImageShared::~SDLImageShared()
{
	VCMI_PROFILE_N("DIAG: image destroyed");
	dropTexture();
	SDL_DestroySurface(surf);
	if (originalPalette)
		SDL_DestroyPalette(originalPalette);
}
