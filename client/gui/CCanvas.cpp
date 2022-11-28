/*
 * CCanvas.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCanvas.h"

#include "SDL_Extensions.h"
#include "Geometries.h"
#include "CAnimation.h"

#include "../Graphics.h"

CCanvas::CCanvas(SDL_Surface * surface):
	surface(surface)
{
	surface->refcount++;
}

CCanvas::CCanvas(const Point & size)
{
	surface = CSDL_Ext::newSurface(size.x, size.y);
}

CCanvas::~CCanvas()
{
	SDL_FreeSurface(surface);
}

void CCanvas::draw(std::shared_ptr<IImage> image, const Point & pos)
{
	image->draw(surface, pos.x, pos.y);
}

void CCanvas::draw(std::shared_ptr<IImage> image, const Point & pos, const Rect & sourceRect)
{
	image->draw(surface, pos.x, pos.y, &sourceRect);
}

void CCanvas::draw(std::shared_ptr<CCanvas> image, const Point & pos)
{
	blitAt(image->surface, pos.x, pos.y, surface);
}

void CCanvas::drawLine(const Point & from, const Point & dest, const SDL_Color & colorFrom, const SDL_Color & colorDest)
{
	CSDL_Ext::drawLine(surface, from.x, from.y, dest.x, dest.y, colorFrom, colorDest);
}

void CCanvas::drawText(const Point & position, const EFonts & font, const SDL_Color & colorDest, ETextAlignment alignment, const std::string & text )
{
	switch (alignment)
	{
	case ETextAlignment::TOPLEFT:      return graphics->fonts[font]->renderTextLeft  (surface, text, colorDest, position);
	case ETextAlignment::CENTER:       return graphics->fonts[font]->renderTextCenter(surface, text, colorDest, position);
	case ETextAlignment::BOTTOMRIGHT:  return graphics->fonts[font]->renderTextRight (surface, text, colorDest, position);
	}
}

void CCanvas::drawText(const Point & position, const EFonts & font, const SDL_Color & colorDest, ETextAlignment alignment, const std::vector<std::string> & text )
{
	switch (alignment)
	{
	case ETextAlignment::TOPLEFT:      return graphics->fonts[font]->renderTextLinesLeft  (surface, text, colorDest, position);
	case ETextAlignment::CENTER:       return graphics->fonts[font]->renderTextLinesCenter(surface, text, colorDest, position);
	case ETextAlignment::BOTTOMRIGHT:  return graphics->fonts[font]->renderTextLinesRight (surface, text, colorDest, position);
	}
}

SDL_Surface * CCanvas::getSurface()
{
	return surface;
}
