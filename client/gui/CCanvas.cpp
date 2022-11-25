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

void CCanvas::draw(std::shared_ptr<CCanvas> image, const Point & pos)
{
	image->copyTo(surface, pos);
}

void CCanvas::drawLine(const Point & from, const Point & dest, const SDL_Color & colorFrom, const SDL_Color & colorDest)
{
	CSDL_Ext::drawLine(surface, from.x, from.y, dest.x, dest.y, colorFrom, colorDest);
}

void CCanvas::copyTo(SDL_Surface * to, const Point & pos)
{
	blitAt(to, pos.x, pos.y, surface);
}
