/*
 * CanvasImage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Profiler.h"
#include "CanvasImage.h"
#include "GpuResources.h"

#include "CMT.h"
#include "GameEngine.h"
#include "IScreenHandler.h"
#include "SDL_Extensions.h"
#include "SDLImageScaler.h"
#include "SDLImage.h"

#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>

CanvasImage::CanvasImage(const Point & size, CanvasScalingPolicy scalingPolicy)
	: surface(CSDL_Ext::newSurface(scalingPolicy == CanvasScalingPolicy::IGNORE ? size : (size * ENGINE->screenHandler().getScalingFactor())))
	, scalingPolicy(scalingPolicy)
{
}

void CanvasImage::invalidateTexture() const
{
	VCMI_PROFILE_N("DIAG: canvas image texture dropped");
	if(texture && textureGeneration == GpuResources::get().generation())
		GpuResources::get().destroyTextureDeferred(texture);

	texture = nullptr;
}

bool CanvasImage::drawTexture(SDL_Renderer * renderer, const Point & pos, const Rect * src, int scalingFactor) const
{
	if(!surface || !renderer)
		return false;

	if(!texture || textureGeneration != GpuResources::get().generation())
	{
		invalidateTexture();
		texture = SDL_CreateTextureFromSurface(renderer, surface);

		if(!texture)
		{
			logGlobal->error("Failed to create texture from canvas image: %s", SDL_GetError());
			return false;
		}

		SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
		textureGeneration = GpuResources::get().generation();
	}

	SDL_FRect source = CSDL_Ext::toSDLFloat(src ? *src : Rect(0, 0, surface->w, surface->h));
	SDL_FRect target{ static_cast<float>(pos.x), static_cast<float>(pos.y), source.w, source.h };

	return SDL_RenderTexture(renderer, texture, &source, &target);
}

CanvasImage::~CanvasImage()
{
	invalidateTexture();
	SDL_DestroySurface(surface);
}

void CanvasImage::draw(SDL_Surface * where, const Point & pos, const Rect * src, int scalingFactor) const
{
	if(src)
		CSDL_Ext::blitSurface(surface, *src, where, pos);
	else
		CSDL_Ext::blitSurface(surface, where, pos);
}

void CanvasImage::scaleTo(const Point & size, EScalingAlgorithm algorithm)
{
	invalidateTexture();

	Point scaledSize = size * ENGINE->screenHandler().getScalingFactor();

	SDLImageScaler scaler(surface);
	scaler.scaleSurface(scaledSize, algorithm);
	SDL_DestroySurface(surface);
	surface = scaler.acquireResultSurface();
}

void CanvasImage::exportBitmap(const boost::filesystem::path & path) const
{
	IMG_SavePNG(surface, path.string().c_str());
}

Canvas CanvasImage::getCanvas()
{
	// the caller is about to draw into the surface, so any GPU copy is now stale
	invalidateTexture();

	return Canvas::createFromSurface(surface, scalingPolicy);
}

Rect CanvasImage::contentRect() const
{
	return Rect(Point(0, 0), dimensions());
}

Point CanvasImage::dimensions() const
{
	if (scalingPolicy != CanvasScalingPolicy::IGNORE)
		return Point(surface->w, surface->h) / ENGINE->screenHandler().getScalingFactor();
	return {surface->w, surface->h};
}

std::shared_ptr<ISharedImage> CanvasImage::toSharedImage()
{
	return std::make_shared<SDLImageShared>(surface);
}
