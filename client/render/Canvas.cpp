/*
 * Canvas.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Canvas.h"

#include "DirtyRegionTracker.h"

#include "../GameEngine.h"
#include "../media/IVideoPlayer.h"
#include "IRenderHandler.h"
#include "IScreenHandler.h"
#include "../renderSDL/SDL_Extensions.h"
#include "Colors.h"
#include "IImage.h"
#include "Graphics.h"
#include "IFont.h"

#include <SDL_surface.h>
#include <SDL_pixels.h>

/// Slack added around reported image areas. Several dimensions() implementations
/// divide a physical size by the scaling factor, so multiplying back can land up to
/// (scalingFactor - 1) pixels short of what is actually written.
static constexpr int imageDirtyMargin = 1;

/// Slack added around reported text areas. Glyph rendering may overshoot the nominal
/// box slightly (shadows, outlines), and getStringWidth() rounds down when converting
/// scaled metrics back to logical ones.
static constexpr int textDirtyMargin = 2;

Canvas::Canvas(SDL_Surface * surface, CanvasScalingPolicy scalingPolicy):
	scalingPolicy(scalingPolicy),
	surface(surface),
	renderArea(0,0, surface->w, surface->h)
{
	surface->refcount++;
}

Canvas::Canvas(const Canvas & other):
	scalingPolicy(other.scalingPolicy),
	surface(other.surface),
	renderArea(other.renderArea)
{
	surface->refcount++;
}

Canvas::Canvas(Canvas && other):
	scalingPolicy(other.scalingPolicy),
	surface(other.surface),
	renderArea(other.renderArea)
{
	surface->refcount++;
}

Canvas::Canvas(const Canvas & other, const Rect & newClipRect):
	Canvas(other)
{
	Rect scaledClipRect( transformPos(newClipRect.topLeft()), transformPos(newClipRect.dimensions()));
	renderArea = other.renderArea.intersect(scaledClipRect + other.renderArea.topLeft());
}

Canvas::Canvas(const Point & size, CanvasScalingPolicy scalingPolicy):
	scalingPolicy(scalingPolicy),
	surface(CSDL_Ext::newSurface(size * getScalingFactor())),
	renderArea(Point(0,0), size * getScalingFactor())
{
	CSDL_Ext::fillSurface(surface, CSDL_Ext::toSDL(Colors::TRANSPARENCY) );
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
}

int Canvas::getScalingFactor() const
{
	if (scalingPolicy == CanvasScalingPolicy::IGNORE)
		return 1;
	return ENGINE->screenHandler().getScalingFactor();
}

Point Canvas::transformPos(const Point & input)
{
	return renderArea.topLeft() + input * getScalingFactor();
}

Point Canvas::transformSize(const Point & input)
{
	return input * getScalingFactor();
}

void Canvas::markDirty(const Rect & surfaceArea) const
{
	ScreenDirtyRegions::markDirty(surface, surfaceArea);
}

void Canvas::markDirtyAll() const
{
	ScreenDirtyRegions::markDirty(surface, renderArea);
}

Canvas Canvas::createFromSurface(SDL_Surface * surface, CanvasScalingPolicy scalingPolicy)
{
	return Canvas(surface, scalingPolicy);
}

void Canvas::applyTransparency(bool on)
{
	if (on)
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
	else
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
}

void Canvas::applyGrayscale()
{
	markDirtyAll();
	CSDL_Ext::convertToGrayscale(surface, renderArea);
}

Canvas::~Canvas()
{
	SDL_FreeSurface(surface);
}

void Canvas::draw(IVideoInstance & video, const Point & pos)
{
	// CVideoInstance::show() blits at pos * scalingFactor and ignores this canvas'
	// render area, and size() rounds the true dimensions down. Report the union of
	// both possible origins with a margin so no case is under-reported.
	Point videoSize = transformSize(video.size()) + Point(getScalingFactor(), getScalingFactor());
	markDirty(Rect(pos * getScalingFactor(), videoSize));
	markDirty(Rect(transformPos(pos), videoSize));

	video.show(pos, surface);
}

void Canvas::draw(const IImage& image, const Point & pos)
{
	markDirty(Rect(transformPos(pos), transformSize(image.dimensions())).resize(imageDirtyMargin * getScalingFactor()));
	image.draw(surface, transformPos(pos), nullptr, getScalingFactor());
}

void Canvas::draw(const std::shared_ptr<IImage>& image, const Point & pos)
{
	assert(image);
	if (image)
	{
		markDirty(Rect(transformPos(pos), transformSize(image->dimensions())).resize(imageDirtyMargin * getScalingFactor()));
		image->draw(surface, transformPos(pos), nullptr, getScalingFactor());
	}
}

void Canvas::draw(const std::shared_ptr<IImage>& image, const Point & pos, const Rect & sourceRect)
{
	Rect realSourceRect = sourceRect * getScalingFactor();
	assert(image);
	if (image)
	{
		markDirty(Rect(transformPos(pos), transformSize(sourceRect.dimensions())).resize(imageDirtyMargin * getScalingFactor()));
		image->draw(surface, transformPos(pos), &realSourceRect, getScalingFactor());
	}
}

void Canvas::draw(const Canvas & image, const Point & pos)
{
	markDirty(Rect(transformPos(pos), image.renderArea.dimensions()));
	CSDL_Ext::blitSurface(image.surface, image.renderArea, surface, transformPos(pos));
}

void Canvas::drawTransparent(const Canvas & image, const Point & pos, double transparency)
{
	SDL_BlendMode oldMode;

	markDirty(Rect(transformPos(pos), image.renderArea.dimensions()));

	SDL_GetSurfaceBlendMode(image.surface, &oldMode);
	SDL_SetSurfaceBlendMode(image.surface, SDL_BLENDMODE_BLEND);
	SDL_SetSurfaceAlphaMod(image.surface, 255 * transparency);
	CSDL_Ext::blitSurface(image.surface, image.renderArea, surface, transformPos(pos));
	SDL_SetSurfaceAlphaMod(image.surface, 255);
	SDL_SetSurfaceBlendMode(image.surface, oldMode);
}

void Canvas::drawScaled(const Canvas & image, const Point & pos, const Point & targetSize)
{
	// SDL_BlitScaled is a software scaler - expensive on large areas.
	Rect targetArea(transformPos(pos), transformSize(targetSize));
	markDirty(targetArea);

	SDL_Rect targetRect = CSDL_Ext::toSDL(targetArea);
	SDL_BlitScaled(image.surface, nullptr, surface, &targetRect);
}

void Canvas::drawPoint(const Point & dest, const ColorRGBA & color)
{
	Point point = transformPos(dest);
	markDirty(Rect(point, Point(1, 1)));
	CSDL_Ext::putPixelWithoutRefreshIfInSurf(surface, point.x, point.y, color.r, color.g, color.b, color.a);
}

void Canvas::drawLine(const Point & from, const Point & dest, const ColorRGBA & colorFrom, const ColorRGBA & colorDest)
{
	Point start = transformPos(from);
	Point end = transformPos(dest);
	Point topLeft(std::min(start.x, end.x), std::min(start.y, end.y));
	Point bottomRight(std::max(start.x, end.x), std::max(start.y, end.y));
	// line is drawn with a thickness of one scaled pixel, hence the extra margin
	markDirty(Rect(topLeft, bottomRight - topLeft).resize(getScalingFactor()));

	CSDL_Ext::drawLine(surface, start, end, CSDL_Ext::toSDL(colorFrom), CSDL_Ext::toSDL(colorDest), getScalingFactor());
}

void Canvas::drawBorder(const Rect & target, const ColorRGBA & color, int width)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	markDirty(realTarget.resize(width * getScalingFactor()));
	CSDL_Ext::drawBorder(surface, realTarget.x, realTarget.y, realTarget.w, realTarget.h, CSDL_Ext::toSDL(color), width * getScalingFactor());
}

void Canvas::drawBorderDashed(const Rect & target, const ColorRGBA & color)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	markDirty(realTarget.resize(getScalingFactor()));
	CSDL_Ext::drawLineDashed(surface, realTarget.topLeft(),    realTarget.topRight(),    CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.bottomLeft(), realTarget.bottomRight(), CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.topLeft(),    realTarget.bottomLeft(),  CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.topRight(),   realTarget.bottomRight(), CSDL_Ext::toSDL(color));
}

void Canvas::drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::string & text )
{
	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);

	{
		// Mirrors the offsets applied by IFont::renderText*, using the scaled metrics
		// so that no glyph can fall outside the reported area.
		Point origin = transformPos(position);
		Point size(fontPtr->getStringWidthScaled(text), fontPtr->getLineHeightScaled());
		Point topLeft = origin;

		if (alignment == ETextAlignment::TOPCENTER || alignment == ETextAlignment::CENTER)
			topLeft = origin - size / 2;
		else if (alignment == ETextAlignment::BOTTOMRIGHT)
			topLeft = origin - size;

		markDirty(Rect(topLeft, size).resize(textDirtyMargin * getScalingFactor()));
	}

	switch (alignment)
	{
	case ETextAlignment::TOPLEFT:      return fontPtr->renderTextLeft  (surface, text, colorDest, transformPos(position));
	case ETextAlignment::TOPCENTER:    return fontPtr->renderTextCenter(surface, text, colorDest, transformPos(position));
	case ETextAlignment::CENTER:       return fontPtr->renderTextCenter(surface, text, colorDest, transformPos(position));
	case ETextAlignment::BOTTOMRIGHT:  return fontPtr->renderTextRight (surface, text, colorDest, transformPos(position));
	}
}

void Canvas::drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::vector<std::string> & text )
{
	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);

	if (!text.empty())
	{
		// Mirrors the offsets applied by IFont::renderTextLines*
		int lineHeight = fontPtr->getLineHeightScaled();
		int maxWidth = 0;
		for (const auto & line : text)
			maxWidth = std::max(maxWidth, static_cast<int>(fontPtr->getStringWidthScaled(line)));

		Point origin = transformPos(position);
		Point size(maxWidth, static_cast<int>(text.size()) * lineHeight);
		Point topLeft = origin;

		if (alignment == ETextAlignment::TOPCENTER || alignment == ETextAlignment::CENTER)
			topLeft = Point(origin.x - size.x / 2, origin.y - size.y / 2 - lineHeight / 2);
		else if (alignment == ETextAlignment::BOTTOMRIGHT)
			topLeft = Point(origin.x - size.x, origin.y - size.y - lineHeight);

		markDirty(Rect(topLeft, size).resize(textDirtyMargin * getScalingFactor()));
	}

	switch (alignment)
	{
	case ETextAlignment::TOPLEFT:      return fontPtr->renderTextLinesLeft  (surface, text, colorDest, transformPos(position));
	case ETextAlignment::TOPCENTER:    return fontPtr->renderTextLinesCenter(surface, text, colorDest, transformPos(position));
	case ETextAlignment::CENTER:       return fontPtr->renderTextLinesCenter(surface, text, colorDest, transformPos(position));
	case ETextAlignment::BOTTOMRIGHT:  return fontPtr->renderTextLinesRight (surface, text, colorDest, transformPos(position));
	}
}

void Canvas::drawColor(const Rect & target, const ColorRGBA & color)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	markDirty(realTarget);
	CSDL_Ext::fillRect(surface, realTarget, CSDL_Ext::toSDL(color));
}

void Canvas::drawColorBlended(const Rect & target, const ColorRGBA & color)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	markDirty(realTarget);
	CSDL_Ext::fillRectBlended(surface, realTarget, CSDL_Ext::toSDL(color));
}

void Canvas::fillTexture(const std::shared_ptr<IImage>& image)
{
	// Tiles one image across the whole canvas - cost scales with canvas area.
	assert(image);
	if (!image)
		return;
		
	// The tiling loop below iterates over the *surface* dimensions and additionally
	// scales the offsets, so it can write well past renderArea (SDL clips the rest).
	// Report the whole surface rather than just this canvas' area.
	markDirty(Rect(0, 0, surface->w, surface->h));

	Rect imageArea(Point(0, 0), image->dimensions());
	for (int y=0; y < surface->h; y+= imageArea.h)
	{
		for (int x=0; x < surface->w; x+= imageArea.w)
			image->draw(surface, Point(renderArea.x + x * getScalingFactor(), renderArea.y + y * getScalingFactor()), nullptr, getScalingFactor());
	}
}

Rect Canvas::getRenderArea() const
{
	return renderArea;
}

ColorRGBA Canvas::getPixel(const Point & position) const
{
	SDL_Color color;
	SDL_GetRGBA(CSDL_Ext::getPixel(surface, position.x, position.y), surface->format, &color.r, &color.g, &color.b, &color.a);
	return ColorRGBA(color.r, color.g, color.b, color.a);
}

CanvasClipRectGuard::CanvasClipRectGuard(Canvas & canvas, const Rect & rect): surf(canvas.surface)
{
	CSDL_Ext::getClipRect(surf, oldRect);
	const Rect scaled = rect * ENGINE->screenHandler().getScalingFactor();
	CSDL_Ext::setClipRect(surf, oldRect.intersect(scaled));
}

CanvasClipRectGuard::~CanvasClipRectGuard()
{
	CSDL_Ext::setClipRect(surf, oldRect);
}
