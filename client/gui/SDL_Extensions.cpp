/*
 * SDL_Extensions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "SDL_Extensions.h"
#include "SDL_Pixels.h"

#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../Graphics.h"
#include "../CMT.h"

#include <SDL_version.h>
#include <SDL_render.h>
#include <SDL_video.h>
#include <SDL_events.h>

#ifdef VCMI_APPLE
#include <dispatch/dispatch.h>
#endif

#ifdef VCMI_IOS
#include "ios/utils.h"
#endif

const SDL_Color Colors::YELLOW = { 229, 215, 123, SDL_ALPHA_OPAQUE };
const SDL_Color Colors::WHITE = { 255, 243, 222, SDL_ALPHA_OPAQUE };
const SDL_Color Colors::METALLIC_GOLD = { 173, 142, 66, SDL_ALPHA_OPAQUE };
const SDL_Color Colors::GREEN = { 0, 255, 0, SDL_ALPHA_OPAQUE };
const SDL_Color Colors::ORANGE = { 232, 184, 32, SDL_ALPHA_OPAQUE };
const SDL_Color Colors::BRIGHT_YELLOW = { 242, 226, 110, SDL_ALPHA_OPAQUE };
const SDL_Color Colors::DEFAULT_KEY_COLOR = {0, 255, 255, SDL_ALPHA_OPAQUE};
const SDL_Color Colors::RED = {255, 0, 0, SDL_ALPHA_OPAQUE};
const SDL_Color Colors::PURPLE = {255, 75, 125, SDL_ALPHA_OPAQUE};
const SDL_Color Colors::BLACK = {0, 0, 0, SDL_ALPHA_OPAQUE};
const SDL_Color Colors::TRANSPARENCY = {0, 0, 0, SDL_ALPHA_TRANSPARENT};

Rect CSDL_Ext::genRect(const int & hh, const int & ww, const int & xx, const int & yy)
{
	Rect ret;
	ret.h=hh;
	ret.w=ww;
	ret.x=xx;
	ret.y=yy;
	return ret;
}

Rect CSDL_Ext::fromSDL(const SDL_Rect & rect)
{
	return Rect(Point(rect.x, rect.y), Point(rect.w, rect.h));
}

SDL_Rect CSDL_Ext::toSDL(const Rect & rect)
{
	SDL_Rect result;
	result.x = rect.x;
	result.y = rect.y;
	result.w = rect.w;
	result.h = rect.h;
	return result;
}

ColorRGBA CSDL_Ext::fromSDL(const SDL_Color & color)
{
	return { color.r, color.g, color.b, color.a };
}

SDL_Color CSDL_Ext::toSDL(const ColorRGBA & color)
{
	SDL_Color result;
	result.r = color.r;
	result.g = color.g;
	result.b = color.b;
	result.a = color.a;

	return result;
}

void CSDL_Ext::setColors(SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors)
{
	SDL_SetPaletteColors(surface->format->palette,colors,firstcolor,ncolors);
}

void CSDL_Ext::warpMouse(int x, int y)
{
	SDL_WarpMouseInWindow(mainWindow,x,y);
}

bool CSDL_Ext::isCtrlKeyDown()
{
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RCTRL];
}

bool CSDL_Ext::isAltKeyDown()
{
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LALT] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RALT];
}

bool CSDL_Ext::isShiftKeyDown()
{
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RSHIFT];
}

void CSDL_Ext::setAlpha(SDL_Surface * bg, int value)
{
	SDL_SetSurfaceAlphaMod(bg, value);
}

void CSDL_Ext::updateRect(SDL_Surface *surface, const Rect & rect )
{
	SDL_Rect rectSDL = CSDL_Ext::toSDL(rect);
	if(0 !=SDL_UpdateTexture(screenTexture, &rectSDL, surface->pixels, surface->pitch))
		logGlobal->error("%sSDL_UpdateTexture %s", __FUNCTION__, SDL_GetError());

	SDL_RenderClear(mainRenderer);
	if(0 != SDL_RenderCopy(mainRenderer, screenTexture, NULL, NULL))
		logGlobal->error("%sSDL_RenderCopy %s", __FUNCTION__, SDL_GetError());
	SDL_RenderPresent(mainRenderer);

}

SDL_Surface * CSDL_Ext::newSurface(int w, int h, SDL_Surface * mod) //creates new surface, with flags/format same as in surface given
{
	SDL_Surface * ret = SDL_CreateRGBSurface(0,w,h,mod->format->BitsPerPixel,mod->format->Rmask,mod->format->Gmask,mod->format->Bmask,mod->format->Amask);
	if (mod->format->palette)
	{
		assert(ret->format->palette);
		assert(ret->format->palette->ncolors == mod->format->palette->ncolors);
		memcpy(ret->format->palette->colors, mod->format->palette->colors, mod->format->palette->ncolors * sizeof(SDL_Color));
	}
	return ret;
}

SDL_Surface * CSDL_Ext::copySurface(SDL_Surface * mod) //returns copy of given surface
{
	//return SDL_DisplayFormat(mod);
	return SDL_ConvertSurface(mod, mod->format, mod->flags);
}

template<int bpp>
SDL_Surface * CSDL_Ext::createSurfaceWithBpp(int width, int height)
{
	Uint32 rMask = 0, gMask = 0, bMask = 0, aMask = 0;

	Channels::px<bpp>::r.set((Uint8*)&rMask, 255);
	Channels::px<bpp>::g.set((Uint8*)&gMask, 255);
	Channels::px<bpp>::b.set((Uint8*)&bMask, 255);
	Channels::px<bpp>::a.set((Uint8*)&aMask, 255);

	return SDL_CreateRGBSurface(0, width, height, bpp * 8, rMask, gMask, bMask, aMask);
}

void CSDL_Ext::blitAt(SDL_Surface * src, int x, int y, SDL_Surface * dst)
{
	if(!dst)
		dst = screen;
	CSDL_Ext::blitSurface(src, dst, Point(x, y));
}

void CSDL_Ext::blitAt(SDL_Surface * src, const Rect & pos, SDL_Surface * dst)
{
	if (src)
		blitAt(src,pos.x,pos.y,dst);
}

// Vertical flip
SDL_Surface * CSDL_Ext::verticalFlip(SDL_Surface * toRot)
{
	SDL_Surface * ret = SDL_ConvertSurface(toRot, toRot->format, toRot->flags);

	SDL_LockSurface(ret);
	SDL_LockSurface(toRot);

	const int bpp = ret->format->BytesPerPixel;

	char * src = reinterpret_cast<char *>(toRot->pixels);
	char * dst = reinterpret_cast<char *>(ret->pixels);

	for(int i=0; i<ret->h; i++)
	{
		//FIXME: optimization bugged
//		if (bpp == 1)
//		{
//			// much faster for 8-bit surfaces (majority of our data)
//			std::reverse_copy(src, src + toRot->pitch, dst);
//		}
//		else
//		{
			char * srcPxl = src;
			char * dstPxl = dst + ret->w * bpp;

			for(int j=0; j<ret->w; j++)
			{
				dstPxl -= bpp;
				std::copy(srcPxl, srcPxl + bpp, dstPxl);
				srcPxl += bpp;
			}
//		}
		src += toRot->pitch;
		dst += ret->pitch;
	}
	SDL_UnlockSurface(ret);
	SDL_UnlockSurface(toRot);
	return ret;
}

// Horizontal flip
SDL_Surface * CSDL_Ext::horizontalFlip(SDL_Surface * toRot)
{
	SDL_Surface * ret = SDL_ConvertSurface(toRot, toRot->format, toRot->flags);
	SDL_LockSurface(ret);
	SDL_LockSurface(toRot);
	char * src = reinterpret_cast<char *>(toRot->pixels);
	char * dst = reinterpret_cast<char *>(ret->pixels) + ret->h * ret->pitch;

	for(int i=0; i<ret->h; i++)
	{
		dst -= ret->pitch;
		std::copy(src, src + toRot->pitch, dst);
		src += toRot->pitch;
	}
	SDL_UnlockSurface(ret);
	SDL_UnlockSurface(toRot);
	return ret;
}

Uint32 CSDL_Ext::getPixel(SDL_Surface *surface, const int & x, const int & y, bool colorByte)
{
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to retrieve */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp)
	{
	case 1:
		if(colorByte)
			return colorToUint32(surface->format->palette->colors+(*p));
		else
			return *p;

	case 2:
		return *(Uint16 *)p;

	case 3:
			return p[0] | p[1] << 8 | p[2] << 16;

	case 4:
		return *(Uint32 *)p;

	default:
		return 0;       // shouldn't happen, but avoids warnings
	}
}

template<int bpp>
int CSDL_Ext::blit8bppAlphaTo24bppT(const SDL_Surface * src, const Rect & srcRectInput, SDL_Surface * dst, const Point & dstPointInput)
{
	SDL_Rect srcRectInstance = CSDL_Ext::toSDL(srcRectInput);
	SDL_Rect dstRectInstance = CSDL_Ext::toSDL(Rect(dstPointInput, srcRectInput.dimensions()));

	SDL_Rect * srcRect =&srcRectInstance;
	SDL_Rect * dstRect =&dstRectInstance;

	/* Make sure the surfaces aren't locked */
	if ( ! src || ! dst )
	{
		SDL_SetError("SDL_UpperBlit: passed a nullptr surface");
		return -1;
	}

	if ( src->locked || dst->locked )
	{
		SDL_SetError("Surfaces must not be locked during blit");
		return -1;
	}

	if (src->format->BytesPerPixel==1 && (bpp==3 || bpp==4 || bpp==2)) //everything's ok
	{
		SDL_Rect fulldst;
		int srcx, srcy, w, h;

		/* If the destination rectangle is nullptr, use the entire dest surface */
		if ( dstRect == nullptr )
		{
			fulldst.x = fulldst.y = 0;
			dstRect = &fulldst;
		}

		/* clip the source rectangle to the source surface */
		if(srcRect)
		{
			int maxw, maxh;

			srcx = srcRect->x;
			w = srcRect->w;
			if(srcx < 0)
			{
				w += srcx;
				dstRect->x -= srcx;
				srcx = 0;
			}
			maxw = src->w - srcx;
			if(maxw < w)
				w = maxw;

			srcy = srcRect->y;
			h = srcRect->h;
			if(srcy < 0)
			{
					h += srcy;
				dstRect->y -= srcy;
				srcy = 0;
			}
			maxh = src->h - srcy;
			if(maxh < h)
				h = maxh;

		}
		else
		{
			srcx = srcy = 0;
			w = src->w;
			h = src->h;
		}

		/* clip the destination rectangle against the clip rectangle */
		{
			SDL_Rect *clip = &dst->clip_rect;
			int dx, dy;

			dx = clip->x - dstRect->x;
			if(dx > 0)
			{
				w -= dx;
				dstRect->x += dx;
				srcx += dx;
			}
			dx = dstRect->x + w - clip->x - clip->w;
			if(dx > 0)
				w -= dx;

			dy = clip->y - dstRect->y;
			if(dy > 0)
			{
				h -= dy;
				dstRect->y += dy;
				srcy += dy;
			}
			dy = dstRect->y + h - clip->y - clip->h;
			if(dy > 0)
				h -= dy;
		}

		if(w > 0 && h > 0)
		{
			dstRect->w = w;
			dstRect->h = h;

			if(SDL_LockSurface(dst))
				return -1; //if we cannot lock the surface

			const SDL_Color *colors = src->format->palette->colors;
			Uint8 *colory = (Uint8*)src->pixels + srcy*src->pitch + srcx;
			Uint8 *py = (Uint8*)dst->pixels + dstRect->y*dst->pitch + dstRect->x*bpp;

			for(int y=h; y; y--, colory+=src->pitch, py+=dst->pitch)
			{
				Uint8 *color = colory;
				Uint8 *p = py;

				for(int x = w; x; x--)
				{
					const SDL_Color &tbc = colors[*color++]; //color to blit
					ColorPutter<bpp, +1>::PutColorAlphaSwitch(p, tbc.r, tbc.g, tbc.b, tbc.a);
				}
			}
			SDL_UnlockSurface(dst);
		}
	}
	return 0;
}

int CSDL_Ext::blit8bppAlphaTo24bpp(const SDL_Surface * src, const Rect & srcRect, SDL_Surface * dst, const Point & dstPoint)
{
	switch(dst->format->BytesPerPixel)
	{
	case 2: return blit8bppAlphaTo24bppT<2>(src, srcRect, dst, dstPoint);
	case 3: return blit8bppAlphaTo24bppT<3>(src, srcRect, dst, dstPoint);
	case 4: return blit8bppAlphaTo24bppT<4>(src, srcRect, dst, dstPoint);
	default:
		logGlobal->error("%d bpp is not supported!", (int)dst->format->BitsPerPixel);
		return -1;
	}
}

Uint32 CSDL_Ext::colorToUint32(const SDL_Color * color)
{
	Uint32 ret = 0;
	ret+=color->a;
	ret<<=8; //*=256
	ret+=color->b;
	ret<<=8; //*=256
	ret+=color->g;
	ret<<=8; //*=256
	ret+=color->r;
	return ret;
}

void CSDL_Ext::update(SDL_Surface * what)
{
	if(!what)
		return;
	if(0 !=SDL_UpdateTexture(screenTexture, nullptr, what->pixels, what->pitch))
		logGlobal->error("%s SDL_UpdateTexture %s", __FUNCTION__, SDL_GetError());
}

static void drawLineX(SDL_Surface * sur, int x1, int y1, int x2, int y2, const SDL_Color & color1, const SDL_Color & color2)
{
	for(int x = x1; x <= x2; x++)
	{
		float f = float(x - x1) / float(x2 - x1);
		int y = vstd::lerp(y1, y2, f);

		uint8_t r = vstd::lerp(color1.r, color2.r, f);
		uint8_t g = vstd::lerp(color1.g, color2.g, f);
		uint8_t b = vstd::lerp(color1.b, color2.b, f);
		uint8_t a = vstd::lerp(color1.a, color2.a, f);

		Uint8 *p = CSDL_Ext::getPxPtr(sur, x, y);
		ColorPutter<4, 0>::PutColor(p, r,g,b,a);
	}
}

static void drawLineY(SDL_Surface * sur, int x1, int y1, int x2, int y2, const SDL_Color & color1, const SDL_Color & color2)
{
	for(int y = y1; y <= y2; y++)
	{
		float f = float(y - y1) / float(y2 - y1);
		int x = vstd::lerp(x1, x2, f);

		uint8_t r = vstd::lerp(color1.r, color2.r, f);
		uint8_t g = vstd::lerp(color1.g, color2.g, f);
		uint8_t b = vstd::lerp(color1.b, color2.b, f);
		uint8_t a = vstd::lerp(color1.a, color2.a, f);

		Uint8 *p = CSDL_Ext::getPxPtr(sur, x, y);
		ColorPutter<4, 0>::PutColor(p, r,g,b,a);
	}
}

void CSDL_Ext::drawLine(SDL_Surface * sur, int x1, int y1, int x2, int y2, const SDL_Color & color1, const SDL_Color & color2)
{
	int width  = std::abs(x1-x2);
	int height = std::abs(y1-y2);

	if ( width == 0 && height == 0)
	{
		Uint8 *p = CSDL_Ext::getPxPtr(sur, x1, y1);
		ColorPutter<4, 0>::PutColorAlpha(p, color1);
		return;
	}

	if (width > height)
	{
		if ( x1 < x2)
			drawLineX(sur, x1,y1,x2,y2, color1, color2);
		else
			drawLineX(sur, x2,y2,x1,y1, color2, color1);
	}
	else
	{
		if ( y1 < y2)
			drawLineY(sur, x1,y1,x2,y2, color1, color2);
		else
			drawLineY(sur, x2,y2,x1,y1, color2, color1);
	}
}

void CSDL_Ext::drawBorder(SDL_Surface * sur, int x, int y, int w, int h, const SDL_Color &color)
{
	for(int i = 0; i < w; i++)
	{
		CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur,x+i,y,color.r,color.g,color.b);
		CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur,x+i,y+h-1,color.r,color.g,color.b);
	}
	for(int i = 0; i < h; i++)
	{
		CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur,x,y+i,color.r,color.g,color.b);
		CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur,x+w-1,y+i,color.r,color.g,color.b);
	}
}

void CSDL_Ext::drawBorder( SDL_Surface * sur, const Rect &r, const SDL_Color &color )
{
	drawBorder(sur, r.x, r.y, r.w, r.h, color);
}

void CSDL_Ext::drawDashedBorder(SDL_Surface * sur, const Rect &r, const SDL_Color &color)
{
	const int y1 = r.y, y2 = r.y + r.h-1;
	for (int i=0; i<r.w; i++)
	{
		const int x = r.x + i;
		if (i%4 || (i==0))
		{
			CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur, x, y1, color.r, color.g, color.b);
			CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur, x, y2, color.r, color.g, color.b);
		}
	}

	const int x1 = r.x, x2 = r.x + r.w-1;
	for (int i=0; i<r.h; i++)
	{
		const int y = r.y + i;
		if ((i%4) || (i==0))
		{
			CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur, x1, y, color.r, color.g, color.b);
			CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur, x2, y, color.r, color.g, color.b);
		}
	}
}

void CSDL_Ext::setPlayerColor(SDL_Surface * sur, PlayerColor player)
{
	if(player==PlayerColor::UNFLAGGABLE)
		return;
	if(sur->format->BitsPerPixel==8)
	{
		SDL_Color *color = (player == PlayerColor::NEUTRAL
							? graphics->neutralColor
							: &graphics->playerColors[player.getNum()]);
		CSDL_Ext::setColors(sur, color, 5, 1);
	}
	else
		logGlobal->warn("Warning, setPlayerColor called on not 8bpp surface!");
}

CSDL_Ext::TColorPutter CSDL_Ext::getPutterFor(SDL_Surface * const &dest, int incrementing)
{
#define CASE_BPP(BytesPerPixel)							\
case BytesPerPixel:									\
	if(incrementing > 0)								\
		return ColorPutter<BytesPerPixel, 1>::PutColor;	\
	else if(incrementing == 0)							\
		return ColorPutter<BytesPerPixel, 0>::PutColor;	\
	else												\
		return ColorPutter<BytesPerPixel, -1>::PutColor;\
	break;

	switch(dest->format->BytesPerPixel)
	{
		CASE_BPP(2)
		CASE_BPP(3)
		CASE_BPP(4)
	default:
		logGlobal->error("%d bpp is not supported!", (int)dest->format->BitsPerPixel);
		return nullptr;
	}

}

CSDL_Ext::TColorPutterAlpha CSDL_Ext::getPutterAlphaFor(SDL_Surface * const &dest, int incrementing)
{
	switch(dest->format->BytesPerPixel)
	{
		CASE_BPP(2)
		CASE_BPP(3)
		CASE_BPP(4)
	default:
		logGlobal->error("%d bpp is not supported!", (int)dest->format->BitsPerPixel);
		return nullptr;
	}
#undef CASE_BPP
}

Uint8 * CSDL_Ext::getPxPtr(const SDL_Surface * const &srf, const int x, const int y)
{
	return (Uint8 *)srf->pixels + y * srf->pitch + x * srf->format->BytesPerPixel;
}

std::string CSDL_Ext::processStr(std::string str, std::vector<std::string> & tor)
{
	for (size_t i=0; (i<tor.size())&&(boost::find_first(str,"%s")); ++i)
	{
		boost::replace_first(str,"%s",tor[i]);
	}
	return str;
}

bool CSDL_Ext::isTransparent( SDL_Surface * srf, const Point & position )
{
	return isTransparent(srf, position.x, position.y);
}

bool CSDL_Ext::isTransparent( SDL_Surface * srf, int x, int y )
{
	if (x < 0 || y < 0 || x >= srf->w || y >= srf->h)
		return true;

	SDL_Color color;

	SDL_GetRGBA(CSDL_Ext::getPixel(srf, x, y), srf->format, &color.r, &color.g, &color.b, &color.a);

	bool pixelTransparent = color.a < 128;
	bool pixelCyan = (color.r == 0 && color.g == 255 && color.b == 255);

	return pixelTransparent || pixelCyan;
}

void CSDL_Ext::VflipSurf(SDL_Surface * surf)
{
	char buf[4]; //buffer
	int bpp = surf->format->BytesPerPixel;
	for (int y=0; y<surf->h; ++y)
	{
		char * base = (char*)surf->pixels + y * surf->pitch;
		for (int x=0; x<surf->w/2; ++x)
		{
			memcpy(buf, base  + x * bpp, bpp);
			memcpy(base + x * bpp, base + (surf->w - x - 1) * bpp, bpp);
			memcpy(base + (surf->w - x - 1) * bpp, buf, bpp);
		}
	}
}

void CSDL_Ext::putPixelWithoutRefresh(SDL_Surface *ekran, const int & x, const int & y, const Uint8 & R, const Uint8 & G, const Uint8 & B, Uint8 A)
{
	Uint8 *p = getPxPtr(ekran, x, y);
	getPutterFor(ekran, false)(p, R, G, B);

	switch(ekran->format->BytesPerPixel)
	{
	case 2: Channels::px<2>::a.set(p, A); break;
	case 3: Channels::px<3>::a.set(p, A); break;
	case 4: Channels::px<4>::a.set(p, A); break;
	}
}

void CSDL_Ext::putPixelWithoutRefreshIfInSurf(SDL_Surface *ekran, const int & x, const int & y, const Uint8 & R, const Uint8 & G, const Uint8 & B, Uint8 A)
{
	const SDL_Rect & rect = ekran->clip_rect;

	if(x >= rect.x  &&  x < rect.w + rect.x
	&& y >= rect.y  &&  y < rect.h + rect.y)
		CSDL_Ext::putPixelWithoutRefresh(ekran, x, y, R, G, B, A);
}

template<int bpp>
void CSDL_Ext::applyEffectBpp(SDL_Surface * surf, const Rect & rect, int mode )
{
	switch(mode)
	{
	case 0: //sepia
		{
			const int sepiaDepth = 20;
			const int sepiaIntensity = 30;

			for(int xp = rect.x; xp < rect.x + rect.w; ++xp)
			{
				for(int yp = rect.y; yp < rect.y + rect.h; ++yp)
				{
					Uint8 * pixel = (ui8*)surf->pixels + yp * surf->pitch + xp * surf->format->BytesPerPixel;
					int r = Channels::px<bpp>::r.get(pixel);
					int g = Channels::px<bpp>::g.get(pixel);
					int b = Channels::px<bpp>::b.get(pixel);
					int gray = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);

					r = g = b = gray;
					r = r + (sepiaDepth * 2);
					g = g + sepiaDepth;

					if (r>255) r=255;
					if (g>255) g=255;
					if (b>255) b=255;

					// Darken blue color to increase sepia effect
					b -= sepiaIntensity;

					// normalize if out of bounds
					if (b<0) b=0;

					Channels::px<bpp>::r.set(pixel, r);
					Channels::px<bpp>::g.set(pixel, g);
					Channels::px<bpp>::b.set(pixel, b);
				}
			}
		}
		break;
	case 1: //grayscale
		{
			for(int xp = rect.x; xp < rect.x + rect.w; ++xp)
			{
				for(int yp = rect.y; yp < rect.y + rect.h; ++yp)
				{
					Uint8 * pixel = (ui8*)surf->pixels + yp * surf->pitch + xp * surf->format->BytesPerPixel;

					int r = Channels::px<bpp>::r.get(pixel);
					int g = Channels::px<bpp>::g.get(pixel);
					int b = Channels::px<bpp>::b.get(pixel);

					int gray = static_cast<int>(0.299 * r + 0.587 * g + 0.114 *b);
					vstd::amax(gray, 255);

					Channels::px<bpp>::r.set(pixel, gray);
					Channels::px<bpp>::g.set(pixel, gray);
					Channels::px<bpp>::b.set(pixel, gray);
				}
			}
		}
		break;
	default:
		throw std::runtime_error("Unsupported effect!");
	}
}

void CSDL_Ext::applyEffect( SDL_Surface * surf, const Rect & rect, int mode )
{
	switch(surf->format->BytesPerPixel)
	{
		case 2: applyEffectBpp<2>(surf, rect, mode); break;
		case 3: applyEffectBpp<3>(surf, rect, mode); break;
		case 4: applyEffectBpp<4>(surf, rect, mode); break;
	}
}

template<int bpp>
void scaleSurfaceFastInternal(SDL_Surface *surf, SDL_Surface *ret)
{
	const float factorX = float(surf->w) / float(ret->w),
				factorY = float(surf->h) / float(ret->h);

	for(int y = 0; y < ret->h; y++)
	{
		for(int x = 0; x < ret->w; x++)
		{
			//coordinates we want to calculate
			int origX = static_cast<int>(floor(factorX * x)),
				origY = static_cast<int>(floor(factorY * y));

			// Get pointers to source pixels
			Uint8 *srcPtr = (Uint8*)surf->pixels + origY * surf->pitch + origX * bpp;
			Uint8 *destPtr = (Uint8*)ret->pixels + y * ret->pitch + x * bpp;

			memcpy(destPtr, srcPtr, bpp);
		}
	}
}

SDL_Surface * CSDL_Ext::scaleSurfaceFast(SDL_Surface *surf, int width, int height)
{
	if (!surf || !width || !height)
		return nullptr;

	//Same size? return copy - this should more be faster
	if (width == surf->w && height == surf->h)
		return copySurface(surf);

	SDL_Surface *ret = newSurface(width, height, surf);

	switch(surf->format->BytesPerPixel)
	{
		case 1: scaleSurfaceFastInternal<1>(surf, ret); break;
		case 2: scaleSurfaceFastInternal<2>(surf, ret); break;
		case 3: scaleSurfaceFastInternal<3>(surf, ret); break;
		case 4: scaleSurfaceFastInternal<4>(surf, ret); break;
	}
	return ret;
}

template<int bpp>
void scaleSurfaceInternal(SDL_Surface *surf, SDL_Surface *ret)
{
	const float factorX = float(surf->w - 1) / float(ret->w),
				factorY = float(surf->h - 1) / float(ret->h);

	for(int y = 0; y < ret->h; y++)
	{
		for(int x = 0; x < ret->w; x++)
		{
			//coordinates we want to interpolate
			float origX = factorX * x,
				  origY = factorY * y;

			float x1 = floor(origX), x2 = floor(origX+1),
				  y1 = floor(origY), y2 = floor(origY+1);
			//assert( x1 >= 0 && y1 >= 0 && x2 < surf->w && y2 < surf->h);//All pixels are in range

			// Calculate weights of each source pixel
			float w11 = ((origX - x1) * (origY - y1));
			float w12 = ((origX - x1) * (y2 - origY));
			float w21 = ((x2 - origX) * (origY - y1));
			float w22 = ((x2 - origX) * (y2 - origY));
			//assert( w11 + w12 + w21 + w22 > 0.99 && w11 + w12 + w21 + w22 < 1.01);//total weight is ~1.0

			// Get pointers to source pixels
			Uint8 *p11 = (Uint8*)surf->pixels + int(y1) * surf->pitch + int(x1) * bpp;
			Uint8 *p12 = p11 + bpp;
			Uint8 *p21 = p11 + surf->pitch;
			Uint8 *p22 = p21 + bpp;
			// Calculate resulting channels
#define PX(X, PTR) Channels::px<bpp>::X.get(PTR)
			int resR = static_cast<int>(PX(r, p11) * w11 + PX(r, p12) * w12 + PX(r, p21) * w21 + PX(r, p22) * w22);
			int resG = static_cast<int>(PX(g, p11) * w11 + PX(g, p12) * w12 + PX(g, p21) * w21 + PX(g, p22) * w22);
			int resB = static_cast<int>(PX(b, p11) * w11 + PX(b, p12) * w12 + PX(b, p21) * w21 + PX(b, p22) * w22);
			int resA = static_cast<int>(PX(a, p11) * w11 + PX(a, p12) * w12 + PX(a, p21) * w21 + PX(a, p22) * w22);
			//assert(resR < 256 && resG < 256 && resB < 256 && resA < 256);
#undef PX
			Uint8 *dest = (Uint8*)ret->pixels + y * ret->pitch + x * bpp;
			Channels::px<bpp>::r.set(dest, resR);
			Channels::px<bpp>::g.set(dest, resG);
			Channels::px<bpp>::b.set(dest, resB);
			Channels::px<bpp>::a.set(dest, resA);
		}
	}
}

// scaling via bilinear interpolation algorithm.
// NOTE: best results are for scaling in range 50%...200%.
// And upscaling looks awful right now - should be fixed somehow
SDL_Surface * CSDL_Ext::scaleSurface(SDL_Surface *surf, int width, int height)
{
	if (!surf || !width || !height)
		return nullptr;

	if (surf->format->palette)
		return scaleSurfaceFast(surf, width, height);

	//Same size? return copy - this should more be faster
	if (width == surf->w && height == surf->h)
		return copySurface(surf);

	SDL_Surface *ret = newSurface(width, height, surf);

	switch(surf->format->BytesPerPixel)
	{
	case 2: scaleSurfaceInternal<2>(surf, ret); break;
	case 3: scaleSurfaceInternal<3>(surf, ret); break;
	case 4: scaleSurfaceInternal<4>(surf, ret); break;
	}

	return ret;
}

void CSDL_Ext::blitSurface(SDL_Surface * src, const Rect & srcRectInput, SDL_Surface * dst, const Point & dstPoint)
{
	SDL_Rect srcRect = CSDL_Ext::toSDL(srcRectInput);
	SDL_Rect dstRect = CSDL_Ext::toSDL(Rect(dstPoint, srcRectInput.dimensions()));

	int result = SDL_UpperBlit(src, &srcRect, dst, &dstRect);

	if (result != 0)
		logGlobal->error("SDL_UpperBlit failed! %s", SDL_GetError());
}

void CSDL_Ext::blitSurface(SDL_Surface * src, SDL_Surface * dst, const Point & dest)
{
	Rect allSurface( Point(0,0), Point(src->w, src->h));

	blitSurface(src, allSurface, dst, dest);
}

void CSDL_Ext::fillSurface( SDL_Surface *dst, const SDL_Color & color )
{
	Rect allSurface( Point(0,0), Point(dst->w, dst->h));

	fillRect(dst, allSurface, color);
}

void CSDL_Ext::fillRect( SDL_Surface *dst, const Rect & dstrect, const SDL_Color & color )
{
	SDL_Rect newRect = CSDL_Ext::toSDL(dstrect);

	uint32_t sdlColor = SDL_MapRGBA(dst->format, color.r, color.g, color.b, color.a);
	SDL_FillRect(dst, &newRect, sdlColor);
}

SDL_Color CSDL_Ext::makeColor(ui8 r, ui8 g, ui8 b, ui8 a)
{
	SDL_Color ret = {r, g, b, a};
	return ret;
}

void CSDL_Ext::startTextInput(const Rect & whereInput)
{
#ifdef VCMI_APPLE
	dispatch_async(dispatch_get_main_queue(), ^{
#endif

	// TODO ios: looks like SDL bug actually, try fixing there
	auto renderer = SDL_GetRenderer(mainWindow);
	float scaleX, scaleY;
	SDL_Rect viewport;
	SDL_RenderGetScale(renderer, &scaleX, &scaleY);
	SDL_RenderGetViewport(renderer, &viewport);

#ifdef VCMI_IOS
	const auto nativeScale = iOS_utils::screenScale();
	scaleX /= nativeScale;
	scaleY /= nativeScale;
#endif

	SDL_Rect rectInScreenCoordinates;
	rectInScreenCoordinates.x = (viewport.x + whereInput.x) * scaleX;
	rectInScreenCoordinates.y = (viewport.y + whereInput.y) * scaleY;
	rectInScreenCoordinates.w = whereInput.w * scaleX;
	rectInScreenCoordinates.h = whereInput.h * scaleY;

	SDL_SetTextInputRect(&rectInScreenCoordinates);

	if (SDL_IsTextInputActive() == SDL_FALSE)
	{
		SDL_StartTextInput();
	}

#ifdef VCMI_APPLE
	});
#endif
}

void CSDL_Ext::stopTextInput()
{
#ifdef VCMI_APPLE
	dispatch_async(dispatch_get_main_queue(), ^{
#endif

	if (SDL_IsTextInputActive() == SDL_TRUE)
	{
		SDL_StopTextInput();
	}

#ifdef VCMI_APPLE
	});
#endif
}

STRONG_INLINE static uint32_t mapColor(SDL_Surface * surface, SDL_Color color)
{
	return SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
}

void CSDL_Ext::setColorKey(SDL_Surface * surface, SDL_Color color)
{
	uint32_t key = mapColor(surface,color);
	SDL_SetColorKey(surface, SDL_TRUE, key);
}

void CSDL_Ext::setDefaultColorKey(SDL_Surface * surface)
{
	setColorKey(surface, Colors::DEFAULT_KEY_COLOR);
}

void CSDL_Ext::setDefaultColorKeyPresize(SDL_Surface * surface)
{
	uint32_t key = mapColor(surface, Colors::DEFAULT_KEY_COLOR);
	auto & color = surface->format->palette->colors[key];

	// set color key only if exactly such color was found
	if (color.r == Colors::DEFAULT_KEY_COLOR.r && color.g == Colors::DEFAULT_KEY_COLOR.g && color.b == Colors::DEFAULT_KEY_COLOR.b)
	{
		SDL_SetColorKey(surface, SDL_TRUE, key);
		color.a = SDL_ALPHA_TRANSPARENT;
	}
}

void CSDL_Ext::setClipRect(SDL_Surface * src, const Rect & other)
{
	SDL_Rect rect = CSDL_Ext::toSDL(other);

	SDL_SetClipRect(src, &rect);
}

void CSDL_Ext::getClipRect(SDL_Surface * src, Rect & other)
{
	SDL_Rect rect;

	SDL_GetClipRect(src, &rect);

	other = CSDL_Ext::fromSDL(rect);
}



template SDL_Surface * CSDL_Ext::createSurfaceWithBpp<2>(int, int);
template SDL_Surface * CSDL_Ext::createSurfaceWithBpp<3>(int, int);
template SDL_Surface * CSDL_Ext::createSurfaceWithBpp<4>(int, int);

