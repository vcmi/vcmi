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

#include "../GameEngine.h"
#include "../media/IVideoPlayer.h"
#include "../render/IRenderHandler.h"
#include "../render/IScreenHandler.h"
#include "../renderSDL/SDL_Extensions.h"
#include "Colors.h"
#include "IImage.h"
#include "Graphics.h"
#include "IFont.h"

#include <SDL_surface.h>
#include <SDL_pixels.h>

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
	CSDL_Ext::convertToGrayscale(surface, renderArea);
}

Canvas::~Canvas()
{
	SDL_FreeSurface(surface);
}

void Canvas::draw(IVideoInstance & video, const Point & pos)
{
	video.show(pos, surface);
}

void Canvas::draw(const IImage& image, const Point & pos)
{
	image.draw(surface, transformPos(pos), nullptr, getScalingFactor());
}

void Canvas::draw(const std::shared_ptr<IImage>& image, const Point & pos)
{
	assert(image);
	if (image)
		image->draw(surface, transformPos(pos), nullptr, getScalingFactor());
}

void Canvas::draw(const std::shared_ptr<IImage>& image, const Point & pos, const Rect & sourceRect)
{
	Rect realSourceRect = sourceRect * getScalingFactor();
	assert(image);
	if (image)
		image->draw(surface, transformPos(pos), &realSourceRect, getScalingFactor());
}

void Canvas::draw(const Canvas & image, const Point & pos)
{
	CSDL_Ext::blitSurface(image.surface, image.renderArea, surface, transformPos(pos));
}

void Canvas::drawTransparent(const Canvas & image, const Point & pos, double transparency)
{
	SDL_BlendMode oldMode;

	SDL_GetSurfaceBlendMode(image.surface, &oldMode);
	SDL_SetSurfaceBlendMode(image.surface, SDL_BLENDMODE_BLEND);
	SDL_SetSurfaceAlphaMod(image.surface, 255 * transparency);
	CSDL_Ext::blitSurface(image.surface, image.renderArea, surface, transformPos(pos));
	SDL_SetSurfaceAlphaMod(image.surface, 255);
	SDL_SetSurfaceBlendMode(image.surface, oldMode);
}

void Canvas::drawScaled(const Canvas & image, const Point & pos, const Point & targetSize)
{
	SDL_Rect targetRect = CSDL_Ext::toSDL(Rect(transformPos(pos), transformSize(targetSize)));
	SDL_BlitScaled(image.surface, nullptr, surface, &targetRect);
}

void Canvas::drawPoint(const Point & dest, const ColorRGBA & color)
{
	Point point = transformPos(dest);
	CSDL_Ext::putPixelWithoutRefreshIfInSurf(surface, point.x, point.y, color.r, color.g, color.b, color.a);
}

void Canvas::drawLine(const Point & from, const Point & dest, const ColorRGBA & colorFrom, const ColorRGBA & colorDest)
{
	CSDL_Ext::drawLine(surface, transformPos(from), transformPos(dest), CSDL_Ext::toSDL(colorFrom), CSDL_Ext::toSDL(colorDest), getScalingFactor());
}

void Canvas::drawBorder(const Rect & target, const ColorRGBA & color, int width)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	CSDL_Ext::drawBorder(surface, realTarget.x, realTarget.y, realTarget.w, realTarget.h, CSDL_Ext::toSDL(color), width * getScalingFactor());
}

void Canvas::drawBorderDashed(const Rect & target, const ColorRGBA & color)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	CSDL_Ext::drawLineDashed(surface, realTarget.topLeft(),    realTarget.topRight(),    CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.bottomLeft(), realTarget.bottomRight(), CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.topLeft(),    realTarget.bottomLeft(),  CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.topRight(),   realTarget.bottomRight(), CSDL_Ext::toSDL(color));
}

void Canvas::drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::string & text )
{
	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);

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

	CSDL_Ext::fillRect(surface, realTarget, CSDL_Ext::toSDL(color));
}

void Canvas::drawColorBlended(const Rect & target, const ColorRGBA & color)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	CSDL_Ext::fillRectBlended(surface, realTarget, CSDL_Ext::toSDL(color));
}

void Canvas::fillTexture(const std::shared_ptr<IImage>& image)
{
	assert(image);
	if (!image)
		return;
		
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
	CSDL_Ext::setClipRect(surf, rect * ENGINE->screenHandler().getScalingFactor());
}

CanvasClipRectGuard::~CanvasClipRectGuard()
{
	CSDL_Ext::setClipRect(surf, oldRect);
}
