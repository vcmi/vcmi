/*
 * Geometries.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Geometries.h"
#include "../CMT.h"
#include <SDL_events.h>
#include "../../lib/int3.h"

Point::Point(const int3 &a)
	:x(a.x),y(a.y)
{}

Point::Point(const SDL_MouseMotionEvent &a)
	:x(a.x),y(a.y)
{}

Rect Rect::createCentered( int w, int h )
{
	return Rect(screen->w/2 - w/2, screen->h/2 - h/2, w, h);
}

Rect Rect::around(const Rect &r, int width)
{
	return Rect(r.x - width, r.y - width, r.w + width * 2, r.h + width * 2);
}

Rect Rect::centerIn(const Rect &r)
{
	return Rect(r.x + (r.w - w) / 2, r.y + (r.h - h) / 2, w, h);
}

