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
#include "../../lib/GameConstants.h"
#include "../../lib/Rect.h"
#include "../../lib/Color.h"

struct SDL_Rect;
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Surface;
struct SDL_Color;

extern SDL_Window * mainWindow;
extern SDL_Renderer * mainRenderer;
extern SDL_Texture * screenTexture;
extern SDL_Surface * screen, *screen2, *screenBuf;

VCMI_LIB_NAMESPACE_BEGIN

class Rect;
class Point;

VCMI_LIB_NAMESPACE_END

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

void setColors(SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors);
void setAlpha(SDL_Surface * bg, int value);

template<typename IntType>
std::string makeNumberShort(IntType number, IntType maxLength = 3) //the output is a string containing at most 5 characters [4 if positive] (eg. intead 10000 it gives 10k)
{
	IntType max = pow(10, maxLength);
	if (std::abs(number) < max)
		return boost::lexical_cast<std::string>(number);

	std::string symbols = " kMGTPE";
	auto iter = symbols.begin();

	while (number >= max)
	{
		number /= 1000;
		iter++;

		assert(iter != symbols.end());//should be enough even for int64
	}
	return boost::lexical_cast<std::string>(number) + *iter;
}

Rect genRect(const int & hh, const int & ww, const int & xx, const int & yy);

typedef void (*TColorPutter)(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B);
typedef void (*TColorPutterAlpha)(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A);

	void blitAt(SDL_Surface * src, int x, int y, SDL_Surface * dst=screen);
	void blitAt(SDL_Surface * src, const Rect & pos, SDL_Surface * dst=screen);

	void setClipRect(SDL_Surface * src, const Rect & other);
	void getClipRect(SDL_Surface * src, Rect & other);

	void blitSurface(SDL_Surface * src, const Rect & srcRect, SDL_Surface * dst, const Point & dest);
	void blitSurface(SDL_Surface * src, SDL_Surface * dst, const Point & dest);

	void fillSurface(SDL_Surface *dst, const SDL_Color & color);
	void fillRect(SDL_Surface *dst, const Rect & dstrect, const SDL_Color & color);

	void updateRect(SDL_Surface *surface, const Rect & rect);

	void putPixelWithoutRefresh(SDL_Surface *ekran, const int & x, const int & y, const uint8_t & R, const uint8_t & G, const uint8_t & B, uint8_t A = 255);
	void putPixelWithoutRefreshIfInSurf(SDL_Surface *ekran, const int & x, const int & y, const uint8_t & R, const uint8_t & G, const uint8_t & B, uint8_t A = 255);

	SDL_Surface * verticalFlip(SDL_Surface * toRot); //vertical flip
	SDL_Surface * horizontalFlip(SDL_Surface * toRot); //horizontal flip
	uint32_t getPixel(SDL_Surface *surface, const int & x, const int & y, bool colorByte = false);
	bool isTransparent(SDL_Surface * srf, int x, int y); //checks if surface is transparent at given position
	bool isTransparent(SDL_Surface * srf, const Point &  position); //checks if surface is transparent at given position

	uint8_t *getPxPtr(const SDL_Surface * const &srf, const int x, const int y);
	TColorPutter getPutterFor(SDL_Surface  * const &dest, int incrementing); //incrementing: -1, 0, 1
	TColorPutterAlpha getPutterAlphaFor(SDL_Surface  * const &dest, int incrementing); //incrementing: -1, 0, 1

	template<int bpp>
	int blit8bppAlphaTo24bppT(const SDL_Surface * src, const Rect & srcRect, SDL_Surface * dst, const Point & dstPoint); //blits 8 bpp surface with alpha channel to 24 bpp surface
	int blit8bppAlphaTo24bpp(const SDL_Surface * src, const Rect & srcRect, SDL_Surface * dst, const Point & dstPoint); //blits 8 bpp surface with alpha channel to 24 bpp surface
	uint32_t colorTouint32_t(const SDL_Color * color); //little endian only
	SDL_Color makeColor(ui8 r, ui8 g, ui8 b, ui8 a);

	void update(SDL_Surface * what = screen); //updates whole surface (default - main screen)
	void drawLine(SDL_Surface * sur, int x1, int y1, int x2, int y2, const SDL_Color & color1, const SDL_Color & color2);
	void drawBorder(SDL_Surface * sur, int x, int y, int w, int h, const SDL_Color &color, int depth = 1);
	void drawBorder(SDL_Surface * sur, const Rect &r, const SDL_Color &color, int depth = 1);
	void drawDashedBorder(SDL_Surface * sur, const Rect &r, const SDL_Color &color);
	void setPlayerColor(SDL_Surface * sur, PlayerColor player); //sets correct color of flags; -1 for neutral
	std::string processStr(std::string str, std::vector<std::string> & tor); //replaces %s in string
	SDL_Surface * newSurface(int w, int h, SDL_Surface * mod=screen); //creates new surface, with flags/format same as in surface given
	SDL_Surface * copySurface(SDL_Surface * mod); //returns copy of given surface
	template<int bpp>
	SDL_Surface * createSurfaceWithBpp(int width, int height); //create surface with give bits per pixels value
	void VflipSurf(SDL_Surface * surf); //fluipis given surface by vertical axis

	//scale surface to required size.
	//nearest neighbour algorithm
	SDL_Surface * scaleSurfaceFast(SDL_Surface *surf, int width, int height);
	// bilinear filtering. Uses fallback to scaleSurfaceFast in case of indexed surfaces
	SDL_Surface * scaleSurface(SDL_Surface *surf, int width, int height);

	template<int bpp>
	void applyEffectBpp( SDL_Surface * surf, const Rect & rect, int mode );
	void applyEffect(SDL_Surface * surf, const Rect & rect, int mode); //mode: 0 - sepia, 1 - grayscale

	void setColorKey(SDL_Surface * surface, SDL_Color color);

	///set key-color to 0,255,255
	void setDefaultColorKey(SDL_Surface * surface);

	///set key-color to 0,255,255 only if it exactly mapped
	void setDefaultColorKeyPresize(SDL_Surface * surface);

	/// helper that will safely set and un-set ClipRect for SDL_Surface
	class CClipRectGuard
	{
		SDL_Surface * surf;
		Rect oldRect;
	public:
		CClipRectGuard(SDL_Surface * surface, const Rect & rect):
			surf(surface)
		{
			CSDL_Ext::getClipRect(surf, oldRect);
			CSDL_Ext::setClipRect(surf, rect);
		}

		~CClipRectGuard()
		{
			CSDL_Ext::setClipRect(surf, oldRect);
		}
	};
}
