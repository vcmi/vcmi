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
#include "CanvasImage.h"

#include "../GameEngine.h"
#include "../render/IScreenHandler.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../renderSDL/SDLImageScaler.h"
#include "../renderSDL/SDLImage.h"

#include <SDL_image.h>
#include <SDL_surface.h>

CanvasImage::CanvasImage(const Point & size, CanvasScalingPolicy scalingPolicy)
	: surface(CSDL_Ext::newSurface(scalingPolicy == CanvasScalingPolicy::IGNORE ? size : (size * ENGINE->screenHandler().getScalingFactor())))
	, scalingPolicy(scalingPolicy)
{
}

CanvasImage::~CanvasImage()
{
	SDL_FreeSurface(surface);
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
	Point scaledSize = size * ENGINE->screenHandler().getScalingFactor();

	SDLImageScaler scaler(surface);
	scaler.scaleSurface(scaledSize, algorithm);
	SDL_FreeSurface(surface);
	surface = scaler.acquireResultSurface();
}

void CanvasImage::exportBitmap(const boost::filesystem::path & path) const
{
	IMG_SavePNG(surface, path.string().c_str());
}

Canvas CanvasImage::getCanvas()
{
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
