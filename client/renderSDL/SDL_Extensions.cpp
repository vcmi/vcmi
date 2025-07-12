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

		uint8_t *p = CSDL_Ext::getPxPtr(sur, x, y);
		ColorPutter<4>::PutColor(p, r,g,b,a);
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
		ColorPutter<4>::PutColor(p, r,g,b,a);
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

SDL_Surface* CSDL_Ext::drawOutline(SDL_Surface* source, const SDL_Color& color, int thickness)
{
	if(thickness < 1)
		return nullptr;

	SDL_Surface* sourceSurface = SDL_ConvertSurfaceFormat(source, SDL_PIXELFORMAT_ARGB8888, 0);
	SDL_Surface* destSurface = newSurface(Point(source->w, source->h));

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

	// Go through all transparent pixels and check if they border an opaque region
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			if(getAlpha(x, y) != 0)
				continue; // Skip opaque pixels

			bool isOutline = false;

			for(int dy = -thickness; dy <= thickness && !isOutline; ++dy)
			{
				for(int dx = -thickness; dx <= thickness; ++dx)
				{
					// circular neighborhood
					if(dx * dx + dy * dy > thickness * thickness)
						continue;

					if(getAlpha(x + dx, y + dy) > 0)
					{
						isOutline = true;
						break; // break inner loop only
					}
				}
			}

			if(isOutline)
			{
				Uint32 newPixel = SDL_MapRGBA(destSurface->format, color.r, color.g, color.b, color.a);
				*((Uint32*)destSurface->pixels + y * width + x) = newPixel;
			}
		}
	}

	if(SDL_MUSTLOCK(sourceSurface)) SDL_UnlockSurface(sourceSurface);
	if(SDL_MUSTLOCK(destSurface)) SDL_UnlockSurface(destSurface);

	SDL_FreeSurface(sourceSurface);
	return destSurface;
}

void applyAffineTransform(SDL_Surface* src, SDL_Surface* dst, double a, double b, double c, double d, double tx, double ty)
{
	assert(src->format->format == SDL_PIXELFORMAT_ARGB8888);
	assert(dst->format->format == SDL_PIXELFORMAT_ARGB8888);

	// Lock surfaces for direct pixel access
	if (SDL_MUSTLOCK(src)) SDL_LockSurface(src);
	if (SDL_MUSTLOCK(dst)) SDL_LockSurface(dst);

	// Calculate inverse matrix M_inv for mapping dst -> src
	double det = a * d - b * c;
	if (std::abs(det) < 1e-10)
		throw std::runtime_error("Singular transform matrix!");
	double invDet = 1.0 / det;
	double ia =  d * invDet;
	double ib = -b * invDet;
	double ic = -c * invDet;
	double id =  a * invDet;

	// For each pixel in the destination image
	for(int y = 0; y < dst->h; y++)
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
				auto srcPixels = (Uint32*)src->pixels;
				auto dstPixels = (Uint32*)dst->pixels;

				Uint32 pixel = srcPixels[srcYi * src->w + srcXi];
				dstPixels[y * dst->w + x] = pixel;
			}
			else
			{
				// Outside source bounds: set transparent or black
				auto dstPixels = (Uint32*)dst->pixels;
				dstPixels[y * dst->w + x] = 0x00000000;  // transparent black
			}
		}
	}

	if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
	if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
}

int getLowestNonTransparentY(SDL_Surface* surface)
{
	assert(surface->format->format == SDL_PIXELFORMAT_ARGB8888);

	if(SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);

	int w = surface->w;
	int h = surface->h;
	int bpp = surface->format->BytesPerPixel;
	auto pixels = (Uint8*)surface->pixels;

	for(int y = h - 1; y >= 0; --y)
	{
		Uint8* row = pixels + y * surface->pitch;

		for(int x = 0; x < w; ++x)
		{
			Uint32 pixel = *(Uint32*)(row + x * bpp);

			Uint8 r;
			Uint8 g;
			Uint8 b;
			Uint8 a;
			SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

			if (a > 0)
			{
				if(SDL_MUSTLOCK(surface))
					SDL_UnlockSurface(surface);
				return y;
			}
		}
	}

	if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
	return -1;  // fully transparent
}

void fillAlphaPixelWithRGBA(SDL_Surface* surface, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	assert(surface->format->format == SDL_PIXELFORMAT_ARGB8888);

	if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);

	auto pixels = (Uint32*)surface->pixels;
	int pixelCount = surface->w * surface->h;

	for (int i = 0; i < pixelCount; i++)
	{
		Uint32 pixel = pixels[i];
		Uint8 pr;
		Uint8 pg;
		Uint8 pb;
		Uint8 pa;
		// Extract existing RGBA components using SDL_GetRGBA
		SDL_GetRGBA(pixel, surface->format, &pr, &pg, &pb, &pa);

		Uint32 newPixel = SDL_MapRGBA(surface->format, r, g, b, a);
		if(pa == 0)
			newPixel = SDL_MapRGBA(surface->format, 0, 0, 0, 0);

		pixels[i] = newPixel;
	}

	if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
}

void gaussianBlur(SDL_Surface* surface, int amount)
{
	assert(surface->format->format == SDL_PIXELFORMAT_ARGB8888);

	if (!surface || amount <= 0) return;

	if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);

	int width = surface->w;
	int height = surface->h;
	int pixelCount = width * height;

	auto pixels = static_cast<Uint32*>(surface->pixels);

	std::vector<Uint8> srcR(pixelCount);
	std::vector<Uint8> srcG(pixelCount);
	std::vector<Uint8> srcB(pixelCount);
	std::vector<Uint8> srcA(pixelCount);

	std::vector<Uint8> dstR(pixelCount);
	std::vector<Uint8> dstG(pixelCount);
	std::vector<Uint8> dstB(pixelCount);
	std::vector<Uint8> dstA(pixelCount);

	// Initialize src channels from surface pixels
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			Uint32 pixel = pixels[y * width + x];
			Uint8 r;
			Uint8 g;
			Uint8 b;
			Uint8 a;
			SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

			int idx = y * width + x;
			srcR[idx] = r;
			srcG[idx] = g;
			srcB[idx] = b;
			srcA[idx] = a;
		}
	}

	// 3x3 Gaussian kernel
	std::array<std::array<float, 3>, 3> kernel = {{
		{{1.f/16, 2.f/16, 1.f/16}},
		{{2.f/16, 4.f/16, 2.f/16}},
		{{1.f/16, 2.f/16, 1.f/16}}
	}};

	// Apply the blur 'amount' times for stronger blur
	for (int iteration = 0; iteration < amount; ++iteration)
	{
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				float sumR = 0.f;
				float sumG = 0.f;
				float sumB = 0.f;
				float sumA = 0.f;

				for (int ky = -1; ky <= 1; ++ky)
				{
					for (int kx = -1; kx <= 1; ++kx)
					{
						int nx = x + kx;
						int ny = y + ky;

						// Clamp edges
						if (nx < 0) nx = 0;
						else if (nx >= width) nx = width - 1;
						if (ny < 0) ny = 0;
						else if (ny >= height) ny = height - 1;

						int nIdx = ny * width + nx;
						float kval = kernel[ky + 1][kx + 1];

						sumR += srcR[nIdx] * kval;
						sumG += srcG[nIdx] * kval;
						sumB += srcB[nIdx] * kval;
						sumA += srcA[nIdx] * kval;
					}
				}

				int idx = y * width + x;
				dstR[idx] = static_cast<Uint8>(sumR);
				dstG[idx] = static_cast<Uint8>(sumG);
				dstB[idx] = static_cast<Uint8>(sumB);
				dstA[idx] = static_cast<Uint8>(sumA);
			}
		}
		// Swap src and dst for next iteration (blur chaining)
		srcR.swap(dstR);
		srcG.swap(dstG);
		srcB.swap(dstB);
		srcA.swap(dstA);
	}

	// After final iteration, write back to surface pixels
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			int idx = y * width + x;
			pixels[idx] = SDL_MapRGBA(surface->format, srcR[idx], srcG[idx], srcB[idx], srcA[idx]);
		}
	}

	if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
}

SDL_Surface * CSDL_Ext::drawShadow(SDL_Surface * source, bool doSheer)
{
	SDL_Surface *sourceSurface = SDL_ConvertSurfaceFormat(source, SDL_PIXELFORMAT_ARGB8888, 0);
	SDL_Surface *destSurface = newSurface(Point(source->w, source->h));

	assert(destSurface->format->format == SDL_PIXELFORMAT_ARGB8888);

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
	gaussianBlur(destSurface, 1);

	SDL_FreeSurface(sourceSurface);

	return destSurface;
}
