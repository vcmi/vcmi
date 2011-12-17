#include "StdInc.h"
#include "SRect.h"

extern SDL_Surface * screen;

SRect SRect::createCentered( int w, int h )
{
	return SRect(screen->w/2 - w/2, screen->h/2 - h/2, w, h);
}

SRect SRect::around(const SRect &r, int width /*= 1*/) /*creates rect around another */
{
	return SRect(r.x - width, r.y - width, r.w + width * 2, r.h + width * 2);
}

SRect SRect::centerIn(const SRect &r)
{
	return SRect(r.x + (r.w - w) / 2, r.y + (r.h - h) / 2, w, h);
}