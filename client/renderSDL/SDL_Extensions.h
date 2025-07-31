/*
 * SDL_Extensions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "../../lib/Rect.h"
#include "../../lib/Color.h"

struct SDL_Rect;
struct SDL_Surface;
struct SDL_Color;

namespace CSDL_Ext
{

/// creates Rect using provided rect
Rect fromSDL(const SDL_Rect & rect);

/// creates SDL_Rect using provided rect
SDL_Rect toSDL(const Rect & rect);

/// creates Color using provided SDL_Color
ColorRGBA fromSDL(const SDL_Color & color);

/// creates SDL_Color using provided Color
SDL_Color toSDL(const ColorRGBA & color);

	void blitAt(SDL_Surface * src, int x, int y, SDL_Surface * dst);
	void blitAt(SDL_Surface * src, const Rect & pos, SDL_Surface * dst);

	void setClipRect(SDL_Surface * src, const Rect & other);
	void getClipRect(SDL_Surface * src, Rect & other);

	void blitSurface(SDL_Surface * src, const Rect & srcRect, SDL_Surface * dst, const Point & dest);
	void blitSurface(SDL_Surface * src, SDL_Surface * dst, const Point & dest);

	void fillSurface(SDL_Surface * dst, const SDL_Color & color);
	void fillRect(SDL_Surface * dst, const Rect & dstrect, const SDL_Color & color);
	void fillRectBlended(SDL_Surface * dst, const Rect & dstrect, const SDL_Color & color);

	void putPixelWithoutRefresh(SDL_Surface * ekran, const int & x, const int & y, const uint8_t & R, const uint8_t & G, const uint8_t & B, uint8_t A = 255);
	void putPixelWithoutRefreshIfInSurf(SDL_Surface *ekran, const int & x, const int & y, const uint8_t & R, const uint8_t & G, const uint8_t & B, uint8_t A = 255);

	SDL_Surface * verticalFlip(SDL_Surface * toRot); //vertical flip
	SDL_Surface * horizontalFlip(SDL_Surface * toRot); //horizontal flip
	uint32_t getPixel(SDL_Surface * surface, const int & x, const int & y, bool colorByte = false);

	uint8_t * getPxPtr(const SDL_Surface * const & srf, const int x, const int y);

	template<int bpp, bool useAlpha>
	int blit8bppAlphaTo24bppT(const SDL_Surface * src, const Rect & srcRect, SDL_Surface * dst, const Point & dstPoint, uint8_t alpha); //blits 8 bpp surface with alpha channel to 24 bpp surface
	int blit8bppAlphaTo24bpp(const SDL_Surface * src, const Rect & srcRect, SDL_Surface * dst, const Point & dstPoint, uint8_t alpha); //blits 8 bpp surface with alpha channel to 24 bpp surface
	uint32_t colorTouint32_t(const SDL_Color * color); //little endian only

	void drawLine(SDL_Surface * sur, const Point & from, const Point & dest, const SDL_Color & color1, const SDL_Color & color2, int width);
	void drawLineDashed(SDL_Surface * sur, const Point & from, const Point & dest, const SDL_Color & color);

	void drawBorder(SDL_Surface * sur, int x, int y, int w, int h, const SDL_Color & color, int depth = 1);
	void drawBorder(SDL_Surface * sur, const Rect & r, const SDL_Color & color, int depth = 1);

	SDL_Surface * newSurface(const Point & dimensions, SDL_Surface * mod); //creates new surface, with flags/format same as in surface given
	SDL_Surface * newSurface(const Point & dimensions); //creates new surface, with flags/format same as in screen surface

	void convertToGrayscale(SDL_Surface * surf, const Rect & rect);
	void convertToH2Scheme(SDL_Surface * surf, const Rect & rect);

	void setColorKey(SDL_Surface * surface, SDL_Color color);

	///set key-color to 0,255,255
	void setDefaultColorKey(SDL_Surface * surface);
	///set key-color to 0,255,255 only if it exactly mapped
	void setDefaultColorKeyPresize(SDL_Surface * surface);

	SDL_Surface * drawOutline(SDL_Surface * source, const SDL_Color & color, int thickness);
	SDL_Surface * drawShadow(SDL_Surface * source, bool doSheer);
}
