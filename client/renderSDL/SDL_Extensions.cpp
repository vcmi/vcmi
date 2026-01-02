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

#include "../GameEngine.h"
#include "../render/Graphics.h"
#include "../render/IImage.h"
#include "../render/IScreenHandler.h"
#include "../render/Colors.h"
#include "../CMT.h"
#include "../xBRZ/xbrz.h"

#include "../../lib/GameConstants.h"

#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range2d.h>

#include <SDL_render.h>
#include <SDL_surface.h>
#include <SDL_version.h>

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

SDL_Surface * CSDL_Ext::newSurface(const Point & dimensions)
{
	return newSurface(dimensions, nullptr);
}

SDL_Surface * CSDL_Ext::newSurface(const Point & dimensions, SDL_Surface * mod) //creates new surface, with flags/format same as in surface given
{
	SDL_Surface * ret = nullptr;

	if (mod != nullptr)
		ret = SDL_CreateRGBSurface(0,dimensions.x,dimensions.y,mod->format->BitsPerPixel,mod->format->Rmask,mod->format->Gmask,mod->format->Bmask,mod->format->Amask);
	else
		ret = SDL_CreateRGBSurfaceWithFormat(0,dimensions.x,dimensions.y,32,SDL_PixelFormatEnum::SDL_PIXELFORMAT_ARGB8888);

	if(ret == nullptr)
	{
		const char * error = SDL_GetError();

		std::string messagePattern = "Failed to create SDL Surface of size %d x %d. Reason: %s";
		std::string message = boost::str(boost::format(messagePattern) % dimensions.x % dimensions.y % error);

		throw std::runtime_error(message);
	}

	if (mod && mod->format->palette)
	{
		assert(ret->format->palette);
		assert(ret->format->palette->ncolors >= mod->format->palette->ncolors);
		memcpy(ret->format->palette->colors, mod->format->palette->colors, mod->format->palette->ncolors * sizeof(SDL_Color));
	}
	return ret;
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

SDL_Surface * CSDL_Ext::Rotate90(SDL_Surface * src)
{
	if (!src)
		return nullptr;

	const int w = src->w;
	const int h = src->h;

	SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, h, w, src->format->BitsPerPixel, src->format->format);
	if (!dst)
		return nullptr;

	SDL_LockSurface(src);
	SDL_LockSurface(dst);

	const Uint32* srcPixels = (Uint32*)src->pixels;
	Uint32*       dstPixels = (Uint32*)dst->pixels;

	const int srcPitch = src->pitch / 4;
	const int dstPitch = dst->pitch / 4;

	constexpr int B = 32; // Tile size (32 is nearly always optimal)

	tbb::parallel_for(
		tbb::blocked_range2d<int>(0, h, B, 0, w, B),
		[&](const tbb::blocked_range2d<int>& r)
		{
			const int y0 = r.rows().begin();
			const int y1 = r.rows().end();
			const int x0 = r.cols().begin();
			const int x1 = r.cols().end();

			for (int y = y0; y < y1; ++y)
			{
				const Uint32* srow = srcPixels + y * srcPitch;

				for (int x = x0; x < x1; ++x)
				{
					const int dx = h - 1 - y;
					const int dy = x;

					dstPixels[dx + dy * dstPitch] = srow[x];
				}
			}
		}
	);

	SDL_UnlockSurface(src);
	SDL_UnlockSurface(dst);

	return dst;
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

template<int bpp, bool useAlpha>
int CSDL_Ext::blit8bppAlphaTo24bppT(const SDL_Surface * src, const Rect & srcRectInput, SDL_Surface * dst, const Point & dstPointInput, [[maybe_unused]] uint8_t alpha)
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
		int srcx;
		int srcy;
		int w;
		int h;

		/* If the destination rectangle is nullptr, use the entire dest surface */
		if ( dstRect == nullptr )
		{
			fulldst.x = fulldst.y = 0;
			dstRect = &fulldst;
		}

		/* clip the source rectangle to the source surface */
		if(srcRect)
		{
			int maxw;
			int maxh;

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
			int dx;
			int dy;

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

			for(int y=0; y<h; ++y, colory+=src->pitch, py+=dst->pitch)
			{
				uint8_t *color = colory;
				uint8_t *p = py;

				for(int x = 0; x < w; ++x)
				{
					const SDL_Color &tbc = colors[*color++]; //color to blit
					if constexpr (useAlpha)
						ColorPutter<bpp>::PutColorAlphaSwitch(p, tbc.r, tbc.g, tbc.b, int(alpha) * tbc.a / 255 );
					else
						ColorPutter<bpp>::PutColorAlphaSwitch(p, tbc.r, tbc.g, tbc.b, tbc.a);

					p += bpp;
				}
			}
			SDL_UnlockSurface(dst);
		}
	}
	return 0;
}

int CSDL_Ext::blit8bppAlphaTo24bpp(const SDL_Surface * src, const Rect & srcRect, SDL_Surface * dst, const Point & dstPoint, uint8_t alpha)
{
	if (alpha == SDL_ALPHA_OPAQUE)
	{
		switch(dst->format->BytesPerPixel)
		{
		case 3: return blit8bppAlphaTo24bppT<3, false>(src, srcRect, dst, dstPoint, alpha);
		case 4: return blit8bppAlphaTo24bppT<4, false>(src, srcRect, dst, dstPoint, alpha);
		}
	}
	else
	{
		switch(dst->format->BytesPerPixel)
		{
			case 3: return blit8bppAlphaTo24bppT<3, true>(src, srcRect, dst, dstPoint, alpha);
			case 4: return blit8bppAlphaTo24bppT<4, true>(src, srcRect, dst, dstPoint, alpha);
		}
	}

	logGlobal->error("%d bpp is not supported!", (int)dst->format->BitsPerPixel);
	return -1;
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

		if (x >= 0 && y >= 0 && x < sur->w && y < sur->h)
		{
			uint8_t *p = CSDL_Ext::getPxPtr(sur, x, y);
			ColorPutter<4>::PutColor(p, r,g,b,a);
		}
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

		if (x >= 0 && y >= 0 && x < sur->w && y < sur->h)
		{
			uint8_t *p = CSDL_Ext::getPxPtr(sur, x, y);
			ColorPutter<4>::PutColor(p, r,g,b,a);
		}
	}
}

void CSDL_Ext::drawLine(SDL_Surface * sur, const Point & from, const Point & dest, const SDL_Color & color1, const SDL_Color & color2, int thickness)
{
	//FIXME: duplicated code with drawLineDashed
	int width = std::abs(from.x - dest.x);
	int height = std::abs(from.y - dest.y);

	if(width == 0 && height == 0)
	{
		uint8_t * p = CSDL_Ext::getPxPtr(sur, from.x, from.y);
		ColorPutter<4>::PutColorAlpha(p, color1);
		return;
	}

	for(int i = 0; i < thickness; ++i)
	{
		if(width > height)
		{
			if(from.x < dest.x)
				drawLineX(sur, from.x, from.y + i, dest.x, dest.y + i, color1, color2);
			else
				drawLineX(sur, dest.x, dest.y + i, from.x, from.y + i, color2, color1);
		}
		else
		{
			if(from.y < dest.y)
				drawLineY(sur, from.x + i, from.y, dest.x + i, dest.y, color1, color2);
			else
				drawLineY(sur, dest.x + i, dest.y, from.x + i, from.y, color2, color1);
		}
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

uint8_t * CSDL_Ext::getPxPtr(const SDL_Surface * const &srf, const int x, const int y)
{
	return (uint8_t *)srf->pixels + y * srf->pitch + x * srf->format->BytesPerPixel;
}

void CSDL_Ext::putPixelWithoutRefresh(SDL_Surface *ekran, const int & x, const int & y, const uint8_t & R, const uint8_t & G, const uint8_t & B, uint8_t A)
{
	uint8_t *p = getPxPtr(ekran, x, y);

	switch(ekran->format->BytesPerPixel)
	{
	case 3:
		ColorPutter<3>::PutColor(p, R, G, B);
		Channels::px<3>::a.set(p, A); break;
	case 4:
		ColorPutter<4>::PutColor(p, R, G, B);
		Channels::px<4>::a.set(p, A); break;
	}
}

void CSDL_Ext::putPixelWithoutRefreshIfInSurf(SDL_Surface *ekran, const int & x, const int & y, const uint8_t & R, const uint8_t & G, const uint8_t & B, uint8_t A)
{
	const SDL_Rect & rect = ekran->clip_rect;

	if(x >= rect.x  &&  x < rect.w + rect.x
	&& y >= rect.y  &&  y < rect.h + rect.y)
		CSDL_Ext::putPixelWithoutRefresh(ekran, x, y, R, G, B, A);
}

template<typename Functor>
void loopOverPixel(SDL_Surface * surf, const Rect & rect, Functor functor)
{
	uint8_t * pixels = static_cast<uint8_t*>(surf->pixels);

	tbb::parallel_for(tbb::blocked_range<size_t>(rect.top(), rect.bottom()), [&](const tbb::blocked_range<size_t>& r)
	{
		for(int yp = r.begin(); yp != r.end(); ++yp)
		{
			uint8_t * pixel_from = pixels + yp * surf->pitch + rect.left() * surf->format->BytesPerPixel;
			uint8_t * pixel_dest = pixels + yp * surf->pitch + rect.right() * surf->format->BytesPerPixel;

			for (uint8_t * pixel = pixel_from; pixel < pixel_dest; pixel += surf->format->BytesPerPixel)
			{
				int r = Channels::px<4>::r.get(pixel);
				int g = Channels::px<4>::g.get(pixel);
				int b = Channels::px<4>::b.get(pixel);

				functor(r, g, b);

				Channels::px<4>::r.set(pixel, r);
				Channels::px<4>::g.set(pixel, g);
				Channels::px<4>::b.set(pixel, b);
			}
		}
	});
}

void CSDL_Ext::convertToGrayscale(SDL_Surface * surf, const Rect & rect )
{
	loopOverPixel(surf, rect, [](int &r, int &g, int &b){
		int gray = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);
		r = gray;
		g = gray;
		b = gray;
	});
}

void CSDL_Ext::convertToH2Scheme(SDL_Surface * surf, const Rect & rect )
{
	loopOverPixel(surf, rect, [](int &r, int &g, int &b){
		double gray = 0.3 * r + 0.59 * g + 0.11 * b;
		double factor = 2.0;

		//fast approximation instead of colorspace conversion
		r = static_cast<int>(gray + (r - gray) * factor);
		g = static_cast<int>(gray + (g - gray) * factor);
		b = static_cast<int>(gray + (b - gray) * factor);

		r = std::clamp(r, 0, 255);
		g = std::clamp(g, 0, 255);
		b = std::clamp(b, 0, 255);
	});
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

SDL_Surface* CSDL_Ext::drawOutline(SDL_Surface* sourceSurface, const SDL_Color& color, int thickness)
{
	if(thickness < 1)
		return nullptr;

	SDL_Surface* destSurface = newSurface(Point(sourceSurface->w, sourceSurface->h));

	if(SDL_MUSTLOCK(sourceSurface)) SDL_LockSurface(sourceSurface);
	if(SDL_MUSTLOCK(destSurface)) SDL_LockSurface(destSurface);

	int width = sourceSurface->w;
	int height = sourceSurface->h;

	// Helper lambda to get alpha
	auto getAlpha = [&](int x, int y) -> Uint8 {
		if (x < 0 || x >= width || y < 0 || y >= height)
			return 0;
		Uint32 pixel = *((Uint32*)sourceSurface->pixels + y * width + x);
		Uint8 r;
		Uint8 g;
		Uint8 b;
		Uint8 a;
		SDL_GetRGBA(pixel, sourceSurface->format, &r, &g, &b, &a);
		return a;
	};

	tbb::parallel_for(tbb::blocked_range<size_t>(0, height), [&](const tbb::blocked_range<size_t>& r)
	{
		for (int y = r.begin(); y != r.end(); ++y)
		{
			for (int x = 0; x < width; x++)
			{
				Uint8 alpha = getAlpha(x, y);
				if (alpha != 0)
					continue; // Skip opaque or semi-transparent pixels

				Uint8 maxNearbyAlpha = 0;

				for (int dy = -thickness; dy <= thickness; ++dy)
				{
					for (int dx = -thickness; dx <= thickness; ++dx)
					{
						if (dx * dx + dy * dy > thickness * thickness)
							continue; // circular area

						int nx = x + dx;
						int ny = y + dy;
						if (nx < 0 || ny < 0 || nx >= width || ny >= height)
							continue;

						Uint8 neighborAlpha = getAlpha(nx, ny);
						if (neighborAlpha > maxNearbyAlpha)
							maxNearbyAlpha = neighborAlpha;
					}
				}

				if (maxNearbyAlpha > 0)
				{
					Uint8 finalAlpha = maxNearbyAlpha - alpha; // alpha is 0 here, so effectively maxNearbyAlpha
					Uint32 newPixel = SDL_MapRGBA(destSurface->format, color.r, color.g, color.b, finalAlpha);
					*((Uint32*)destSurface->pixels + y * width + x) = newPixel;
				}
			}
		}
	});

	if(SDL_MUSTLOCK(sourceSurface)) SDL_UnlockSurface(sourceSurface);
	if(SDL_MUSTLOCK(destSurface)) SDL_UnlockSurface(destSurface);

	return destSurface;
}

void applyAffineTransform(SDL_Surface* src, SDL_Surface* dst, double a, double b, double c, double d, double tx, double ty)
{
	// Check if the transform is purely scaling (and optionally translation)
	bool isPureScaling = vstd::isAlmostZero(b) && vstd::isAlmostZero(c);

	if (isPureScaling)
	{
		// Calculate target dimensions
		int scaledW = static_cast<int>(src->w * a);
		int scaledH = static_cast<int>(src->h * d);

		SDL_Rect srcRect = { 0, 0, src->w, src->h };
		SDL_Rect dstRect = { static_cast<int>(tx), static_cast<int>(ty), scaledW, scaledH };

		// Convert surfaces to same format if needed
		if (src->format->format != dst->format->format)
		{
			SDL_Surface* converted = SDL_ConvertSurface(src, dst->format, 0);
			if (!converted)
				throw std::runtime_error("SDL_ConvertSurface failed!");

			SDL_BlitScaled(converted, &srcRect, dst, &dstRect);
			SDL_FreeSurface(converted);
		}
		else
			SDL_BlitScaled(src, &srcRect, dst, &dstRect);

		return;
	}

	// Lock surfaces for direct pixel access
	if (SDL_MUSTLOCK(src)) SDL_LockSurface(src);
	if (SDL_MUSTLOCK(dst)) SDL_LockSurface(dst);

	// Calculate inverse matrix M_inv for mapping dst -> src
	double det = a * d - b * c;
	if (vstd::isAlmostZero(det))
		throw std::runtime_error("Singular transform matrix!");
	double invDet = 1.0 / det;
	double ia =  d * invDet;
	double ib = -b * invDet;
	double ic = -c * invDet;
	double id =  a * invDet;

	auto srcPixels = (Uint32*)src->pixels;
	auto dstPixels = (Uint32*)dst->pixels;

	tbb::parallel_for(tbb::blocked_range<size_t>(0, dst->h), [&](const tbb::blocked_range<size_t>& r)
	{
		// For each pixel in the destination image
		for(int y = r.begin(); y != r.end(); ++y)
		{
			for(int x = 0; x < dst->w; x++)
			{
				// Map destination pixel (x,y) back to source coordinates (srcX, srcY)
				double srcX = ia * (x - tx) + ib * (y - ty);
				double srcY = ic * (x - tx) + id * (y - ty);

				// Nearest neighbor sampling (can be improved to bilinear)
				auto srcXi = static_cast<int>(round(srcX));
				auto srcYi = static_cast<int>(round(srcY));

				// Check bounds
				if (srcXi >= 0 && srcXi < src->w && srcYi >= 0 && srcYi < src->h)
				{
					Uint32 pixel = srcPixels[srcYi * src->w + srcXi];
					dstPixels[y * dst->w + x] = pixel;
				}
				else
					dstPixels[y * dst->w + x] = 0x00000000;  // transparent black
			}
		}
	});

	if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
	if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
}

int getLowestNonTransparentY(SDL_Surface* surface)
{
	if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);

	const int w = surface->w;
	const int h = surface->h;
	const int bpp = surface->format->BytesPerPixel;
	auto pixels = static_cast<Uint8*>(surface->pixels);

	// Use parallel_reduce to find the max y with non-transparent pixel
	int lowestY = tbb::parallel_reduce(
		tbb::blocked_range<int>(0, h),
		-1,  // initial lowestY = -1 (fully transparent)
		[&](const tbb::blocked_range<int>& r, int localMaxY) -> int
		{
			for (int y = r.begin(); y != r.end(); ++y)
			{
				Uint8* row = pixels + y * surface->pitch;
				for (int x = 0; x < w; ++x)
				{
					Uint32 pixel = *(Uint32*)(row + x * bpp);
					Uint8 a = (pixel >> 24) & 0xFF; // Fast path for ARGB8888
					if (a > 0)
					{
						localMaxY = std::max(localMaxY, y);
						break; // no need to scan rest of row
					}
				}
			}
			return localMaxY;
		},
		[](int a, int b) -> int
		{
			return std::max(a, b);
		}
	);

	if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
	return lowestY;
}

void fillAlphaPixelWithRGBA(SDL_Surface* surface, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);

	auto pixels = (Uint32*)surface->pixels;
	int pixelCount = surface->w * surface->h;

	tbb::parallel_for(tbb::blocked_range<size_t>(0, pixelCount), [&](const tbb::blocked_range<size_t>& range)
	{
		for(int i = range.begin(); i != range.end(); ++i)
		{
			Uint32 pixel = pixels[i];
			Uint8 pr;
			Uint8 pg;
			Uint8 pb;
			Uint8 pa;
			// Extract existing RGBA components using SDL_GetRGBA
			SDL_GetRGBA(pixel, surface->format, &pr, &pg, &pb, &pa);

			Uint32 newPixel = SDL_MapRGBA(surface->format, r, g, b, a);
			if(pa < 128)
				newPixel = SDL_MapRGBA(surface->format, 0, 0, 0, 0);

			pixels[i] = newPixel;
		}
	});

	if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
}

void boxBlur(SDL_Surface* surface)
{
	if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);

	int width = surface->w;
	int height = surface->h;
	int pixelCount = width * height;

	Uint32* pixels = static_cast<Uint32*>(surface->pixels);
	std::vector<Uint32> temp(pixelCount);

	tbb::parallel_for(0, height, [&](int y)
	{
		for (int x = 0; x < width; ++x)
		{
			int sumR = 0;
			int sumG = 0;
			int sumB = 0;
			int sumA = 0;
			int count = 0;

			for (int ky = -1; ky <= 1; ++ky)
			{
				int ny = std::clamp(y + ky, 0, height - 1);
				for (int kx = -1; kx <= 1; ++kx)
				{
					int nx = std::clamp(x + kx, 0, width - 1);
					Uint32 pixel = pixels[ny * width + nx];

					sumA += (pixel >> 24) & 0xFF;
					sumR += (pixel >> 16) & 0xFF;
					sumG += (pixel >> 8)  & 0xFF;
					sumB += (pixel >> 0)  & 0xFF;
					++count;
				}
			}

			Uint8 a = sumA / count;
			Uint8 r = sumR / count;
			Uint8 g = sumG / count;
			Uint8 b = sumB / count;
			temp[y * width + x] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	});

	std::copy(temp.begin(), temp.end(), pixels);

	if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
}

SDL_Surface * CSDL_Ext::drawShadow(SDL_Surface * sourceSurface, bool doSheer)
{
	SDL_Surface *destSurface = newSurface(Point(sourceSurface->w, sourceSurface->h));

	double shearX = doSheer ? 0.5 : 0.0;
	double scaleY = doSheer ? 0.5 : 0.25;

	int lowestSource = getLowestNonTransparentY(sourceSurface);
	int lowestTransformed = lowestSource * scaleY;

	// Parameters for applyAffineTransform
	double a = 1.0;
	double b = shearX;
	double c = 0.0;
	double d = scaleY;
	double tx = -shearX * lowestSource;
	double ty = lowestSource - lowestTransformed;

	applyAffineTransform(sourceSurface, destSurface, a, b, c, d, tx, ty);
	fillAlphaPixelWithRGBA(destSurface, 0, 0, 0, 128);
	boxBlur(destSurface);

	return destSurface;
}

void CSDL_Ext::adjustBrightness(SDL_Surface* surface, float factor)
{
    if (!surface || surface->format->BytesPerPixel != 4)
        return;

    SDL_LockSurface(surface);

    Uint8 r;
	Uint8 g;
	Uint8 b;
	Uint8 a;

    for (int y = 0; y < surface->h; y++)
    {
        auto* row = reinterpret_cast<Uint32*>(static_cast<Uint8*>(surface->pixels) + y * surface->pitch);

        for (int x = 0; x < surface->w; x++)
        {
            Uint32 pixel = row[x];

            SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

            r = std::min(255, static_cast<int>(r * factor));
            g = std::min(255, static_cast<int>(g * factor));
            b = std::min(255, static_cast<int>(b * factor));

            row[x] = SDL_MapRGBA(surface->format, r, g, b, a);
        }
    }

    SDL_UnlockSurface(surface);
}
