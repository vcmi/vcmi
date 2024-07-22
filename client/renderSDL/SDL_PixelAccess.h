/*
 * SDL_PixelAccess.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "SDL_Extensions.h"

#include <SDL_endian.h>
#include <SDL_pixels.h>

// for accessing channels from pixel in format-independent way
//should be as fast as accessing via pointer at least for 3-4 bytes per pixel
namespace Channels
{
	// channel not present in this format
	struct channel_empty
	{
		static STRONG_INLINE void set(uint8_t*, uint8_t) {}
		static STRONG_INLINE uint8_t get(const uint8_t *) { return 255;}
	};

	// channel which uses whole pixel
	template<int offset>
	struct channel_pixel
	{
		static STRONG_INLINE void set(uint8_t *ptr, uint8_t value)
		{
			ptr[offset] = value;
		}

		static STRONG_INLINE uint8_t get(const uint8_t *ptr)
		{
			return ptr[offset];
		}
	};

	// channel which uses part of pixel
	template <int bits, int mask, int shift>
	struct channel_subpx
	{

		static void STRONG_INLINE set(uint8_t *ptr, uint8_t value)
		{
			auto * const pixel = (uint16_t *)ptr;
			uint8_t subpx = value >> (8 - bits);
			*pixel = (*pixel & ~mask) | ((subpx << shift) & mask );
		}

		static uint8_t STRONG_INLINE get(const uint8_t *ptr)
		{
			uint8_t subpx = (*((uint16_t *)ptr) & mask) >> shift;
			return (subpx << (8 - bits)) | (subpx >> (2*bits - 8));
		}
	};

	template<int bpp>
	struct px
	{
		static channel_empty r;
		static channel_empty g;
		static channel_empty b;
		static channel_empty a;
	};

#ifdef VCMI_ENDIAN_BIG

	template<>
	struct px<4>
	{
		static channel_pixel<1> r;
		static channel_pixel<2> g;
		static channel_pixel<3> b;
		static channel_pixel<0> a;
	};

	template<>
	struct px<3>
	{
		static channel_pixel<0> r;
		static channel_pixel<1> g;
		static channel_pixel<2> b;
		static channel_empty a;
	};

#else

	template<>
	struct px<4>
	{
		static channel_pixel<2> r;
		static channel_pixel<1> g;
		static channel_pixel<0> b;
		static channel_pixel<3> a;
	};

	template<>
	struct px<3>
	{
		static channel_pixel<2> r;
		static channel_pixel<1> g;
		static channel_pixel<0> b;
		static channel_empty a;
	};

#endif
}

template<int bpp>
struct ColorPutter
{
	static STRONG_INLINE void PutColor(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B);
	static STRONG_INLINE void PutColor(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A);
	static STRONG_INLINE void PutColorAlphaSwitch(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A);
	static STRONG_INLINE void PutColorAlpha(uint8_t *&ptr, const SDL_Color & Color);
};

template<int bpp>
STRONG_INLINE void ColorPutter<bpp>::PutColorAlpha(uint8_t *&ptr, const SDL_Color & Color)
{
	PutColor(ptr, Color.r, Color.g, Color.b, Color.a);
}

template<int bpp>
STRONG_INLINE void ColorPutter<bpp>::PutColorAlphaSwitch(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A)
{
	switch (A)
	{
	case 0:
		return;
	case 255:
		PutColor(ptr, R, G, B);
		return;
	case 128:  // optimized
		PutColor(ptr,
				 ((uint16_t)R + Channels::px<bpp>::r.get(ptr)) >> 1,
				 ((uint16_t)G + Channels::px<bpp>::g.get(ptr)) >> 1,
				 ((uint16_t)B + Channels::px<bpp>::b.get(ptr)) >> 1);
		return;
	default:
		PutColor(ptr, R, G, B, A);
		return;
	}
}

template<int bpp>
STRONG_INLINE void ColorPutter<bpp>::PutColor(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A)
{
	PutColor(ptr, ((((uint32_t)R - (uint32_t)Channels::px<bpp>::r.get(ptr))*(uint32_t)A) >> 8 ) + (uint32_t)Channels::px<bpp>::r.get(ptr),
				  ((((uint32_t)G - (uint32_t)Channels::px<bpp>::g.get(ptr))*(uint32_t)A) >> 8 ) + (uint32_t)Channels::px<bpp>::g.get(ptr),
				  ((((uint32_t)B - (uint32_t)Channels::px<bpp>::b.get(ptr))*(uint32_t)A) >> 8 ) + (uint32_t)Channels::px<bpp>::b.get(ptr));
}

template<int bpp>
STRONG_INLINE void ColorPutter<bpp>::PutColor(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B)
{
	Channels::px<bpp>::r.set(ptr, R);
	Channels::px<bpp>::g.set(ptr, G);
	Channels::px<bpp>::b.set(ptr, B);
	Channels::px<bpp>::a.set(ptr, 255);
}
