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

#include "SDL_PixelAccess.h"

#include "../render/Graphics.h"
#include "../render/Colors.h"
#include "../CMT.h"

#include "../../lib/GameConstants.h"

#include <SDL_render.h>

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
	if(0 != SDL_RenderCopy(mainRenderer, screenTexture, nullptr, nullptr))
		logGlobal->error("%sSDL_RenderCopy %s", __FUNCTION__, SDL_GetError());
	SDL_RenderPresent(mainRenderer);

}

SDL_Surface * CSDL_Ext::newSurface(int w, int h)
{
	return newSurface(w, h, screen);
}

SDL_Surface * CSDL_Ext::newSurface(int w, int h, SDL_Surface * mod) //creates new surface, with flags/format same as in surface given
{
	SDL_Surface * ret = SDL_CreateRGBSurface(0,w,h,mod->format->BitsPerPixel,mod->format->Rmask,mod->format->Gmask,mod->format->Bmask,mod->format->Amask);

	if(ret == nullptr)
	{
		const char * error = SDL_GetError();

		std::string messagePattern = "Failed to create SDL Surface of size %d x %d, %d bpp. Reason: %s";
		std::string message = boost::str(boost::format(messagePattern) % w % h % mod->format->BitsPerPixel % error);

		handleFatalError(message, true);
	}

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
	uint32_t rMask = 0, gMask = 0, bMask = 0, aMask = 0;

	Channels::px<bpp>::r.set((uint8_t*)&rMask, 255);
	Channels::px<bpp>::g.set((uint8_t*)&gMask, 255);
	Channels::px<bpp>::b.set((uint8_t*)&bMask, 255);
	Channels::px<bpp>::a.set((uint8_t*)&aMask, 255);

	return SDL_CreateRGBSurface(0, width, height, bpp * 8, rMask, gMask, bMask, aMask);
}

void CSDL_Ext::blitAt(SDL_Surface * src, int x, int y, SDL_Surface * dst)
{
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

uint32_t CSDL_Ext::getPixel(SDL_Surface *surface, const int & x, const int & y, bool colorByte)
{
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to retrieve */
	uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp)
	{
	case 1:
		if(colorByte)
			return colorTouint32_t(surface->format->palette->colors+(*p));
		else
			return *p;

	case 2:
		return *(uint16_t *)p;

	case 3:
			return p[0] | p[1] << 8 | p[2] << 16;

	case 4:
		return *(uint32_t *)p;

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
			uint8_t *colory = (uint8_t*)src->pixels + srcy*src->pitch + srcx;
			uint8_t *py = (uint8_t*)dst->pixels + dstRect->y*dst->pitch + dstRect->x*bpp;

			for(int y=h; y; y--, colory+=src->pitch, py+=dst->pitch)
			{
				uint8_t *color = colory;
				uint8_t *p = py;

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

uint32_t CSDL_Ext::colorTouint32_t(const SDL_Color * color)
{
	uint32_t ret = 0;
	ret+=color->a;
	ret<<=8; //*=256
	ret+=color->b;
	ret<<=8; //*=256
	ret+=color->g;
	ret<<=8; //*=256
	ret+=color->r;
	return ret;
}

static void drawLineXDashed(SDL_Surface * sur, int x1, int y1, int x2, int y2, const SDL_Color & color)
{
	double length(x2 - x1);

	for(int x = x1; x <= x2; x++)
	{
		double f = (x - x1) / length;
		int y = vstd::lerp(y1, y2, f);

		if (std::abs(x - x1) % 5 != 4)
			CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur, x, y, color.r, color.g, color.b);
	}
}

static void drawLineYDashed(SDL_Surface * sur, int x1, int y1, int x2, int y2, const SDL_Color & color)
{
	double length(y2 - y1);

	for(int y = y1; y <= y2; y++)
	{
		double f = (y - y1) / length;
		int x = vstd::lerp(x1, x2, f);

		if (std::abs(y - y1) % 5 != 4)
			CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur, x, y, color.r, color.g, color.b);
	}
}

static void drawLineX(SDL_Surface * sur, int x1, int y1, int x2, int y2, const SDL_Color & color1, const SDL_Color & color2)
{
	double length(x2 - x1);
	for(int x = x1; x <= x2; x++)
	{
		double f = (x - x1) / length;
		int y = vstd::lerp(y1, y2, f);

		uint8_t r = vstd::lerp(color1.r, color2.r, f);
		uint8_t g = vstd::lerp(color1.g, color2.g, f);
		uint8_t b = vstd::lerp(color1.b, color2.b, f);
		uint8_t a = vstd::lerp(color1.a, color2.a, f);

		uint8_t *p = CSDL_Ext::getPxPtr(sur, x, y);
		ColorPutter<4, 0>::PutColor(p, r,g,b,a);
	}
}

static void drawLineY(SDL_Surface * sur, int x1, int y1, int x2, int y2, const SDL_Color & color1, const SDL_Color & color2)
{
	double length(y2 - y1);
	for(int y = y1; y <= y2; y++)
	{
		double f = (y - y1) / length;
		int x = vstd::lerp(x1, x2, f);

		uint8_t r = vstd::lerp(color1.r, color2.r, f);
		uint8_t g = vstd::lerp(color1.g, color2.g, f);
		uint8_t b = vstd::lerp(color1.b, color2.b, f);
		uint8_t a = vstd::lerp(color1.a, color2.a, f);

		uint8_t *p = CSDL_Ext::getPxPtr(sur, x, y);
		ColorPutter<4, 0>::PutColor(p, r,g,b,a);
	}
}

void CSDL_Ext::drawLine(SDL_Surface * sur, const Point & from, const Point & dest, const SDL_Color & color1, const SDL_Color & color2)
{
	//FIXME: duplicated code with drawLineDashed
	int width  = std::abs(from.x - dest.x);
	int height = std::abs(from.y - dest.y);

	if ( width == 0 && height == 0)
	{
		uint8_t *p = CSDL_Ext::getPxPtr(sur, from.x, from.y);
		ColorPutter<4, 0>::PutColorAlpha(p, color1);
		return;
	}

	if (width > height)
	{
		if ( from.x < dest.x)
			drawLineX(sur, from.x, from.y, dest.x, dest.y, color1, color2);
		else
			drawLineX(sur, dest.x, dest.y, from.x, from.y, color2, color1);
	}
	else
	{
		if ( from.y < dest.y)
			drawLineY(sur, from.x, from.y, dest.x, dest.y, color1, color2);
		else
			drawLineY(sur, dest.x, dest.y, from.x, from.y, color2, color1);
	}
}

void CSDL_Ext::drawLineDashed(SDL_Surface * sur, const Point & from, const Point & dest, const SDL_Color & color)
{
	//FIXME: duplicated code with drawLine
	int width  = std::abs(from.x - dest.x);
	int height = std::abs(from.y - dest.y);

	if ( width == 0 && height == 0)
	{
		CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur, from.x, from.y, color.r, color.g, color.b);
		return;
	}

	if (width > height)
	{
		if ( from.x < dest.x)
			drawLineXDashed(sur, from.x, from.y, dest.x, dest.y, color);
		else
			drawLineXDashed(sur, dest.x, dest.y, from.x, from.y, color);
	}
	else
	{
		if ( from.y < dest.y)
			drawLineYDashed(sur, from.x, from.y, dest.x, dest.y, color);
		else
			drawLineYDashed(sur, dest.x, dest.y, from.x, from.y, color);
	}
}

void CSDL_Ext::drawBorder(SDL_Surface * sur, int x, int y, int w, int h, const SDL_Color &color, int depth)
{
	depth = std::max(1, depth);
	for(int depthIterator = 0; depthIterator < depth; depthIterator++)
	{
		for(int i = 0; i < w; i++)
		{
			CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur,x+i,y+depthIterator,color.r,color.g,color.b);
			CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur,x+i,y+h-1-depthIterator,color.r,color.g,color.b);
		}
		for(int i = 0; i < h; i++)
		{
			CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur,x+depthIterator,y+i,color.r,color.g,color.b);
			CSDL_Ext::putPixelWithoutRefreshIfInSurf(sur,x+w-1-depthIterator,y+i,color.r,color.g,color.b);
		}
	}
}

void CSDL_Ext::drawBorder( SDL_Surface * sur, const Rect &r, const SDL_Color &color, int depth)
{
	drawBorder(sur, r.x, r.y, r.w, r.h, color, depth);
}

void CSDL_Ext::setPlayerColor(SDL_Surface * sur, const PlayerColor & player)
{
	if(player==PlayerColor::UNFLAGGABLE)
		return;
	if(sur->format->BitsPerPixel==8)
	{
		ColorRGBA color = (player == PlayerColor::NEUTRAL
							? graphics->neutralColor
							: graphics->playerColors[player.getNum()]);

		SDL_Color colorSDL = toSDL(color);
		CSDL_Ext::setColors(sur, &colorSDL, 5, 1);
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

uint8_t * CSDL_Ext::getPxPtr(const SDL_Surface * const &srf, const int x, const int y)
{
	return (uint8_t *)srf->pixels + y * srf->pitch + x * srf->format->BytesPerPixel;
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

void CSDL_Ext::putPixelWithoutRefresh(SDL_Surface *ekran, const int & x, const int & y, const uint8_t & R, const uint8_t & G, const uint8_t & B, uint8_t A)
{
	uint8_t *p = getPxPtr(ekran, x, y);
	getPutterFor(ekran, false)(p, R, G, B);

	switch(ekran->format->BytesPerPixel)
	{
	case 2: Channels::px<2>::a.set(p, A); break;
	case 3: Channels::px<3>::a.set(p, A); break;
	case 4: Channels::px<4>::a.set(p, A); break;
	}
}

void CSDL_Ext::putPixelWithoutRefreshIfInSurf(SDL_Surface *ekran, const int & x, const int & y, const uint8_t & R, const uint8_t & G, const uint8_t & B, uint8_t A)
{
	const SDL_Rect & rect = ekran->clip_rect;

	if(x >= rect.x  &&  x < rect.w + rect.x
	&& y >= rect.y  &&  y < rect.h + rect.y)
		CSDL_Ext::putPixelWithoutRefresh(ekran, x, y, R, G, B, A);
}

template<int bpp>
void CSDL_Ext::convertToGrayscaleBpp(SDL_Surface * surf, const Rect & rect )
{
	uint8_t * pixels = static_cast<uint8_t*>(surf->pixels);

	for(int yp = rect.top(); yp < rect.bottom(); ++yp)
	{
		uint8_t * pixel_from = pixels + yp * surf->pitch + rect.left() * surf->format->BytesPerPixel;
		uint8_t * pixel_dest = pixels + yp * surf->pitch + rect.right() * surf->format->BytesPerPixel;

		for (uint8_t * pixel = pixel_from; pixel < pixel_dest; pixel += surf->format->BytesPerPixel)
		{
			int r = Channels::px<bpp>::r.get(pixel);
			int g = Channels::px<bpp>::g.get(pixel);
			int b = Channels::px<bpp>::b.get(pixel);

			int gray = static_cast<int>(0.299 * r + 0.587 * g + 0.114 *b);

			Channels::px<bpp>::r.set(pixel, gray);
			Channels::px<bpp>::g.set(pixel, gray);
			Channels::px<bpp>::b.set(pixel, gray);
		}
	}
}

void CSDL_Ext::convertToGrayscale( SDL_Surface * surf, const Rect & rect )
{
	switch(surf->format->BytesPerPixel)
	{
		case 2: convertToGrayscaleBpp<2>(surf, rect); break;
		case 3: convertToGrayscaleBpp<3>(surf, rect); break;
		case 4: convertToGrayscaleBpp<4>(surf, rect); break;
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
			uint8_t *srcPtr = (uint8_t*)surf->pixels + origY * surf->pitch + origX * bpp;
			uint8_t *destPtr = (uint8_t*)ret->pixels + y * ret->pitch + x * bpp;

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
			uint8_t *p11 = (uint8_t*)surf->pixels + int(y1) * surf->pitch + int(x1) * bpp;
			uint8_t *p12 = p11 + bpp;
			uint8_t *p21 = p11 + surf->pitch;
			uint8_t *p22 = p21 + bpp;
			// Calculate resulting channels
#define PX(X, PTR) Channels::px<bpp>::X.get(PTR)
			int resR = static_cast<int>(PX(r, p11) * w11 + PX(r, p12) * w12 + PX(r, p21) * w21 + PX(r, p22) * w22);
			int resG = static_cast<int>(PX(g, p11) * w11 + PX(g, p12) * w12 + PX(g, p21) * w21 + PX(g, p22) * w22);
			int resB = static_cast<int>(PX(b, p11) * w11 + PX(b, p12) * w12 + PX(b, p21) * w21 + PX(b, p22) * w22);
			int resA = static_cast<int>(PX(a, p11) * w11 + PX(a, p12) * w12 + PX(a, p21) * w21 + PX(a, p22) * w22);
			//assert(resR < 256 && resG < 256 && resB < 256 && resA < 256);
#undef PX
			uint8_t *dest = (uint8_t*)ret->pixels + y * ret->pitch + x * bpp;
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

void CSDL_Ext::fillRect( SDL_Surface *dst, const Rect & dstrect, const SDL_Color & color)
{
	SDL_Rect newRect = CSDL_Ext::toSDL(dstrect);

	uint32_t sdlColor = SDL_MapRGBA(dst->format, color.r, color.g, color.b, color.a);
	SDL_FillRect(dst, &newRect, sdlColor);
}

void CSDL_Ext::fillRectBlended( SDL_Surface *dst, const Rect & dstrect, const SDL_Color & color)
{
	SDL_Rect newRect = CSDL_Ext::toSDL(dstrect);
	uint32_t sdlColor = SDL_MapRGBA(dst->format, color.r, color.g, color.b, color.a);

	SDL_Surface * tmp = SDL_CreateRGBSurface(0, newRect.w, newRect.h, dst->format->BitsPerPixel, dst->format->Rmask, dst->format->Gmask, dst->format->Bmask, dst->format->Amask);
	SDL_FillRect(tmp, nullptr, sdlColor);
	SDL_BlitSurface(tmp, nullptr, dst, &newRect);
	SDL_FreeSurface(tmp);
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
	setColorKey(surface, toSDL(Colors::DEFAULT_KEY_COLOR));
}

void CSDL_Ext::setDefaultColorKeyPresize(SDL_Surface * surface)
{
	uint32_t key = mapColor(surface, toSDL(Colors::DEFAULT_KEY_COLOR));
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

