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

#include "SDL_Extensions.h"
#include "CAnimation.h"

#include "../Graphics.h"

#include <SDL_surface.h>

Canvas::Canvas(SDL_Surface * surface):
	surface(surface),
	renderOffset(0,0)
{
	surface->refcount++;
}

Canvas::Canvas(Canvas & other):
	surface(other.surface),
	renderOffset(other.renderOffset)
{
	surface->refcount++;
}

Canvas::Canvas(Canvas & other, const Rect & newClipRect):
	Canvas(other)
{
	clipRect.emplace();
	CSDL_Ext::getClipRect(surface, clipRect.get());

	Rect currClipRect = newClipRect + renderOffset;
	CSDL_Ext::setClipRect(surface, currClipRect);

	renderOffset += newClipRect.topLeft();
}

Canvas::Canvas(const Point & size):
	renderOffset(0,0)
{
	surface = CSDL_Ext::newSurface(size.x, size.y);
}

Canvas::~Canvas()
{
	if (clipRect)
		CSDL_Ext::setClipRect(surface, clipRect.get());

	SDL_FreeSurface(surface);
}

void Canvas::draw(std::shared_ptr<IImage> image, const Point & pos)
{
	assert(image);
	if (image)
		image->draw(surface, renderOffset.x + pos.x, renderOffset.y + pos.y);
}

void Canvas::draw(std::shared_ptr<IImage> image, const Point & pos, const Rect & sourceRect)
{
	assert(image);
	if (image)
		image->draw(surface, renderOffset.x + pos.x, renderOffset.y + pos.y, &sourceRect);
}

void Canvas::draw(Canvas & image, const Point & pos)
{
	CSDL_Ext::blitAt(image.surface, renderOffset.x + pos.x, renderOffset.y + pos.y, surface);
}

void Canvas::drawLine(const Point & from, const Point & dest, const SDL_Color & colorFrom, const SDL_Color & colorDest)
{
	CSDL_Ext::drawLine(surface, renderOffset.x + from.x, renderOffset.y + from.y, renderOffset.x + dest.x, renderOffset.y + dest.y, colorFrom, colorDest);
}

void Canvas::drawText(const Point & position, const EFonts & font, const SDL_Color & colorDest, ETextAlignment alignment, const std::string & text )
{
	switch (alignment)
	{
	case ETextAlignment::TOPLEFT:      return graphics->fonts[font]->renderTextLeft  (surface, text, colorDest, renderOffset + position);
	case ETextAlignment::CENTER:       return graphics->fonts[font]->renderTextCenter(surface, text, colorDest, renderOffset + position);
	case ETextAlignment::BOTTOMRIGHT:  return graphics->fonts[font]->renderTextRight (surface, text, colorDest, renderOffset + position);
	}
}

void Canvas::drawText(const Point & position, const EFonts & font, const SDL_Color & colorDest, ETextAlignment alignment, const std::vector<std::string> & text )
{
	switch (alignment)
	{
	case ETextAlignment::TOPLEFT:      return graphics->fonts[font]->renderTextLinesLeft  (surface, text, colorDest, renderOffset + position);
	case ETextAlignment::CENTER:       return graphics->fonts[font]->renderTextLinesCenter(surface, text, colorDest, renderOffset + position);
	case ETextAlignment::BOTTOMRIGHT:  return graphics->fonts[font]->renderTextLinesRight (surface, text, colorDest, renderOffset + position);
	}
}

