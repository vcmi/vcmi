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

#include <SDL_rect.h>
#include <SDL_events.h>

Rect Geometry::fromSDL(const SDL_Rect & rect)
{
	return Rect(Point(rect.x, rect.y), Point(rect.w, rect.h));
}

SDL_Rect Geometry::toSDL(const Rect & rect)
{
	SDL_Rect result;
	result.x = rect.x;
	result.y = rect.y;
	result.w = rect.w;
	result.h = rect.h;
	return result;
}

Point Geometry::fromSDL(const SDL_MouseMotionEvent & motion)
{
	return { motion.x, motion.y };
}
