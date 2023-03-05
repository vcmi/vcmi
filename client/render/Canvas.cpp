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

#include "../renderSDL/SDL_Extensions.h"
#include "Colors.h"
#include "IImage.h"
#include "Graphics.h"

#include <SDL_surface.h>

Canvas::Canvas(SDL_Surface * surface):
	surface(surface),
	renderArea(0,0, surface->w, surface->h)
{
	surface->refcount++;
}

Canvas::Canvas(const Canvas & other):
	surface(other.surface),
	renderArea(other.renderArea)
{
	surface->refcount++;
}

Canvas::Canvas(const Canvas & other, const Rect & newClipRect):
	Canvas(other)
{
	renderArea = other.renderArea.intersect(newClipRect + other.renderArea.topLeft());
}

Canvas::Canvas(const Point & size):
	renderArea(Point(0,0), size),
	surface(CSDL_Ext::newSurface(size.x, size.y))
{
	CSDL_Ext::fillSurface(surface, Colors::TRANSPARENCY );
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
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

void Canvas::draw(const std::shared_ptr<IImage>& image, const Point & pos)
{
	assert(image);
	if (image)
		image->draw(surface, renderArea.x + pos.x, renderArea.y + pos.y);
}

void Canvas::draw(const std::shared_ptr<IImage>& image, const Point & pos, const Rect & sourceRect)
{
	assert(image);
	if (image)
		image->draw(surface, renderArea.x + pos.x, renderArea.y + pos.y, &sourceRect);
}

void Canvas::draw(const Canvas & image, const Point & pos)
{
	CSDL_Ext::blitSurface(image.surface, image.renderArea, surface, renderArea.topLeft() + pos);
}

void Canvas::drawTransparent(const Canvas & image, const Point & pos, double transparency)
{
	SDL_BlendMode oldMode;

	SDL_GetSurfaceBlendMode(image.surface, &oldMode);
	SDL_SetSurfaceBlendMode(image.surface, SDL_BLENDMODE_BLEND);
	SDL_SetSurfaceAlphaMod(image.surface, 255 * transparency);
	CSDL_Ext::blitSurface(image.surface, image.renderArea, surface, renderArea.topLeft() + pos);
	SDL_SetSurfaceAlphaMod(image.surface, 255);
	SDL_SetSurfaceBlendMode(image.surface, oldMode);
}

void Canvas::drawScaled(const Canvas & image, const Point & pos, const Point & targetSize)
{
	SDL_Rect targetRect = CSDL_Ext::toSDL(Rect(pos + renderArea.topLeft(), targetSize));
	SDL_BlitScaled(image.surface, nullptr, surface, &targetRect);
}

void Canvas::drawPoint(const Point & dest, const ColorRGBA & color)
{
	CSDL_Ext::putPixelWithoutRefreshIfInSurf(surface, dest.x, dest.y, color.r, color.g, color.b, color.a);
}

void Canvas::drawLine(const Point & from, const Point & dest, const ColorRGBA & colorFrom, const ColorRGBA & colorDest)
{
	CSDL_Ext::drawLine(surface, renderArea.topLeft() + from, renderArea.topLeft() + dest, CSDL_Ext::toSDL(colorFrom), CSDL_Ext::toSDL(colorDest));
}

void Canvas::drawLineDashed(const Point & from, const Point & dest, const ColorRGBA & color)
{
	CSDL_Ext::drawLineDashed(surface, renderArea.topLeft() + from, renderArea.topLeft() + dest, CSDL_Ext::toSDL(color));
}

void Canvas::drawBorderDashed(const Rect & target, const ColorRGBA & color)
{
	Rect realTarget = target + renderArea.topLeft();

	CSDL_Ext::drawLineDashed(surface, realTarget.topLeft(),    realTarget.topRight(),    CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.bottomLeft(), realTarget.bottomRight(), CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.topLeft(),    realTarget.bottomLeft(),  CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.topRight(),   realTarget.bottomRight(), CSDL_Ext::toSDL(color));
}

void Canvas::drawText(const Point & position, const EFonts & font, const SDL_Color & colorDest, ETextAlignment alignment, const std::string & text )
{
	switch (alignment)
	{
	case ETextAlignment::TOPLEFT:      return graphics->fonts[font]->renderTextLeft  (surface, text, colorDest, renderArea.topLeft() + position);
	case ETextAlignment::CENTER:       return graphics->fonts[font]->renderTextCenter(surface, text, colorDest, renderArea.topLeft() + position);
	case ETextAlignment::BOTTOMRIGHT:  return graphics->fonts[font]->renderTextRight (surface, text, colorDest, renderArea.topLeft() + position);
	}
}

void Canvas::drawText(const Point & position, const EFonts & font, const SDL_Color & colorDest, ETextAlignment alignment, const std::vector<std::string> & text )
{
	switch (alignment)
	{
	case ETextAlignment::TOPLEFT:      return graphics->fonts[font]->renderTextLinesLeft  (surface, text, colorDest, renderArea.topLeft() + position);
	case ETextAlignment::CENTER:       return graphics->fonts[font]->renderTextLinesCenter(surface, text, colorDest, renderArea.topLeft() + position);
	case ETextAlignment::BOTTOMRIGHT:  return graphics->fonts[font]->renderTextLinesRight (surface, text, colorDest, renderArea.topLeft() + position);
	}
}

