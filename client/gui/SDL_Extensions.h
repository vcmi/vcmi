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
#include <SDL_version.h>
#include <SDL_render.h>
#include <SDL_video.h>
#include <SDL_events.h>
#include "../../lib/int3.h"
#include "Geometries.h"
#include "../../lib/GameConstants.h"


extern SDL_Window * mainWindow;
extern SDL_Renderer * mainRenderer;
extern SDL_Texture * screenTexture;

inline void SDL_SetColors(SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors)
{
	SDL_SetPaletteColors(surface->format->palette,colors,firstcolor,ncolors);
}

inline void SDL_WarpMouse(int x, int y)
{
	SDL_WarpMouseInWindow(mainWindow,x,y);
}

void SDL_UpdateRect(SDL_Surface *surface, int x, int y, int w, int h);

inline bool isCtrlKeyDown()
{
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RCTRL];
}

inline bool isAltKeyDown()
{
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LALT] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RALT];
}

inline bool isShiftKeyDown()
{
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RSHIFT];
}
namespace CSDL_Ext
{
	template<typename Int>
	Int lerp(Int a, Int b, float f)
	{
		return a + std::round((b - a) * f);
	}

	//todo: should this better be assignment operator?
	STRONG_INLINE void colorAssign(SDL_Color & dest, const SDL_Color & source)
	{
		dest.r = source.r;
		dest.g = source.g;
		dest.b = source.b;
		dest.a = source.a;
	}

	inline void setAlpha(SDL_Surface * bg, int value)
	{
		SDL_SetSurfaceAlphaMod(bg, value);
	}
}
struct Rect;

extern SDL_Surface * screen, *screen2, *screenBuf;
void blitAt(SDL_Surface * src, int x, int y, SDL_Surface * dst=screen);
void blitAt(SDL_Surface * src, const SDL_Rect & pos, SDL_Surface * dst=screen);
bool isItIn(const SDL_Rect * rect, int x, int y);
bool isItInOrLowerBounds(const SDL_Rect * rect, int x, int y);

/**
 * The colors class defines color constants of type SDL_Color.
 */
class Colors
{
public:
	/** the h3 yellow color, typically used for headlines */
	static const SDL_Color YELLOW;

	/** the standard h3 white color */
	static const SDL_Color WHITE;

	/** the metallic gold color used mostly as a border around buttons */
	static const SDL_Color METALLIC_GOLD;

	/** green color used for in-game console */
	static const SDL_Color GREEN;

	/** the h3 orange color, used for blocked buttons */
	static const SDL_Color ORANGE;

	/** the h3 bright yellow color, used for selection border */
	static const SDL_Color BRIGHT_YELLOW;

	/** default key color for all 8 & 24 bit graphics */
	static const SDL_Color DEFAULT_KEY_COLOR;
};

//MSVC gives an error when calling abs with ui64 -> we add template that will match calls with unsigned arg and return it
template<typename T>
typename boost::enable_if_c<boost::is_unsigned<T>::type, T>::type abs(T arg)
{
	return arg;
}

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

typedef void (*TColorPutter)(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B);
typedef void (*TColorPutterAlpha)(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B, const Uint8 & A);

inline SDL_Rect genRect(const int & hh, const int & ww, const int & xx, const int & yy)
{
	SDL_Rect ret;
	ret.h=hh;
	ret.w=ww;
	ret.x=xx;
	ret.y=yy;
	return ret;
}

template<int bpp, int incrementPtr>
struct ColorPutter
{
	static STRONG_INLINE void PutColor(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B);
	static STRONG_INLINE void PutColor(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B, const Uint8 & A);
	static STRONG_INLINE void PutColorAlphaSwitch(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B, const Uint8 & A);
	static STRONG_INLINE void PutColor(Uint8 *&ptr, const SDL_Color & Color);
	static STRONG_INLINE void PutColorAlpha(Uint8 *&ptr, const SDL_Color & Color);
	static STRONG_INLINE void PutColorRow(Uint8 *&ptr, const SDL_Color & Color, size_t count);
};

typedef void (*BlitterWithRotationVal)(SDL_Surface *src,SDL_Rect srcRect, SDL_Surface * dst, SDL_Rect dstRect, ui8 rotation);

/// Base class for applying palette transformation on images
class ColorShifter
{
public:
	~ColorShifter() = default;
	virtual SDL_Color shiftColor(SDL_Color input) const = 0;
};

/// Generic class for palette transformation
/// formula:
/// result = input * factor + added
class ColorShifterMultiplyAndAdd : public ColorShifter
{
	SDL_Color added;
	SDL_Color factor;

public:
	ColorShifterMultiplyAndAdd(SDL_Color factor, SDL_Color added) :
		factor(factor),
		added(added)
	{}

	SDL_Color shiftColor(SDL_Color input) const override
	{
		return {
			uint8_t(std::min(255.f, std::round(input.r * float(factor.r) / 255.f + added.r))),
			uint8_t(std::min(255.f, std::round(input.g * float(factor.g) / 255.f + added.g))),
			uint8_t(std::min(255.f, std::round(input.b * float(factor.b) / 255.f + added.b))),
			uint8_t(std::min(255.f, std::round(input.a * float(factor.a) / 255.f + added.a))),
		};
	}
};

/// Color shifter that allows to specify color to be excempt from changes
class ColorShifterMultiplyAndAddExcept : public ColorShifterMultiplyAndAdd
{
	SDL_Color ignored;
public:
	ColorShifterMultiplyAndAddExcept(SDL_Color factor, SDL_Color added, SDL_Color ignored) :
		ColorShifterMultiplyAndAdd(factor, added),
		ignored(ignored)
	{}

	SDL_Color shiftColor(SDL_Color input) const override
	{
		if ( input.r == ignored.r && input.g == ignored.g && input.b == ignored.b && input.a == ignored.a)
			return input;
		return ColorShifterMultiplyAndAdd::shiftColor(input);
	}
};

namespace CSDL_Ext
{
	/// helper that will safely set and un-set ClipRect for SDL_Surface
	class CClipRectGuard
	{
		SDL_Surface * surf;
		SDL_Rect oldRect;
	public:
		CClipRectGuard(SDL_Surface * surface, const SDL_Rect & rect):
			surf(surface)
		{
			SDL_GetClipRect(surf, &oldRect);
			SDL_SetClipRect(surf, &rect);
		}

		~CClipRectGuard()
		{
			SDL_SetClipRect(surf, &oldRect);
		}
	};

	void blitSurface(SDL_Surface * src, const SDL_Rect * srcRect, SDL_Surface * dst, SDL_Rect * dstRect);
	void fillRect(SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color);
	void fillRectBlack(SDL_Surface * dst, SDL_Rect * dstrect);
	//fill dest image with source texture.
	void fillTexture(SDL_Surface *dst, SDL_Surface * sourceTexture);

	void SDL_PutPixelWithoutRefresh(SDL_Surface *ekran, const int & x, const int & y, const Uint8 & R, const Uint8 & G, const Uint8 & B, Uint8 A = 255);
	void SDL_PutPixelWithoutRefreshIfInSurf(SDL_Surface *ekran, const int & x, const int & y, const Uint8 & R, const Uint8 & G, const Uint8 & B, Uint8 A = 255);

	SDL_Surface * verticalFlip(SDL_Surface * toRot); //vertical flip
	SDL_Surface * horizontalFlip(SDL_Surface * toRot); //horizontal flip
	Uint32 SDL_GetPixel(SDL_Surface *surface, const int & x, const int & y, bool colorByte = false);
	bool isTransparent(SDL_Surface * srf, int x, int y); //checks if surface is transparent at given position

	Uint8 *getPxPtr(const SDL_Surface * const &srf, const int x, const int y);
	TColorPutter getPutterFor(SDL_Surface  * const &dest, int incrementing); //incrementing: -1, 0, 1
	TColorPutterAlpha getPutterAlphaFor(SDL_Surface  * const &dest, int incrementing); //incrementing: -1, 0, 1

	template<int bpp>
	int blit8bppAlphaTo24bppT(const SDL_Surface * src, const SDL_Rect * srcRect, SDL_Surface * dst, SDL_Rect * dstRect); //blits 8 bpp surface with alpha channel to 24 bpp surface
	int blit8bppAlphaTo24bpp(const SDL_Surface * src, const SDL_Rect * srcRect, SDL_Surface * dst, SDL_Rect * dstRect); //blits 8 bpp surface with alpha channel to 24 bpp surface
	Uint32 colorToUint32(const SDL_Color * color); //little endian only
	SDL_Color makeColor(ui8 r, ui8 g, ui8 b, ui8 a);

	void update(SDL_Surface * what = screen); //updates whole surface (default - main screen)
	void drawLine(SDL_Surface * sur, int x1, int y1, int x2, int y2, const SDL_Color & color1, const SDL_Color & color2);
	void drawBorder(SDL_Surface * sur, int x, int y, int w, int h, const int3 &color);
	void drawBorder(SDL_Surface * sur, const SDL_Rect &r, const int3 &color);
	void drawDashedBorder(SDL_Surface * sur, const Rect &r, const int3 &color);
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
	void applyEffectBpp( SDL_Surface * surf, const SDL_Rect * rect, int mode );
	void applyEffect(SDL_Surface * surf, const SDL_Rect * rect, int mode); //mode: 0 - sepia, 1 - grayscale

	void startTextInput(SDL_Rect * where);
	void stopTextInput();

	void setColorKey(SDL_Surface * surface, SDL_Color color);

	///set key-color to 0,255,255
	void setDefaultColorKey(SDL_Surface * surface);

	///set key-color to 0,255,255 only if it exactly mapped
	void setDefaultColorKeyPresize(SDL_Surface * surface);
}
