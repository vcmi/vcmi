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

	template<>
	struct px<2>
	{
		static channel_subpx<5, 0xF800, 11> r;
		static channel_subpx<6, 0x07E0, 5 > g;
		static channel_subpx<5, 0x001F, 0 > b;
		static channel_empty a;
	};
}

template<int bpp, int incrementPtr>
struct ColorPutter
{
	static STRONG_INLINE void PutColor(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B);
	static STRONG_INLINE void PutColor(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A);
	static STRONG_INLINE void PutColorAlphaSwitch(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A);
	static STRONG_INLINE void PutColor(uint8_t *&ptr, const SDL_Color & Color);
	static STRONG_INLINE void PutColorAlpha(uint8_t *&ptr, const SDL_Color & Color);
	static STRONG_INLINE void PutColorRow(uint8_t *&ptr, const SDL_Color & Color, size_t count);
};

template <int incrementPtr>
struct ColorPutter<2, incrementPtr>
{
	static STRONG_INLINE void PutColor(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B);
	static STRONG_INLINE void PutColor(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A);
	static STRONG_INLINE void PutColorAlphaSwitch(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A);
	static STRONG_INLINE void PutColor(uint8_t *&ptr, const SDL_Color & Color);
	static STRONG_INLINE void PutColorAlpha(uint8_t *&ptr, const SDL_Color & Color);
	static STRONG_INLINE void PutColorRow(uint8_t *&ptr, const SDL_Color & Color, size_t count);
};

template<int bpp, int incrementPtr>
STRONG_INLINE void ColorPutter<bpp, incrementPtr>::PutColorAlpha(uint8_t *&ptr, const SDL_Color & Color)
{
	PutColor(ptr, Color.r, Color.g, Color.b, Color.a);
}

template<int bpp, int incrementPtr>
STRONG_INLINE void ColorPutter<bpp, incrementPtr>::PutColor(uint8_t *&ptr, const SDL_Color & Color)
{
	PutColor(ptr, Color.r, Color.g, Color.b);
}

template<int bpp, int incrementPtr>
STRONG_INLINE void ColorPutter<bpp, incrementPtr>::PutColorAlphaSwitch(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A)
{
	switch (A)
	{
	case 0:
		ptr += bpp * incrementPtr;
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

template<int bpp, int incrementPtr>
STRONG_INLINE void ColorPutter<bpp, incrementPtr>::PutColor(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A)
{
	PutColor(ptr, ((((uint32_t)R - (uint32_t)Channels::px<bpp>::r.get(ptr))*(uint32_t)A) >> 8 ) + (uint32_t)Channels::px<bpp>::r.get(ptr),
				  ((((uint32_t)G - (uint32_t)Channels::px<bpp>::g.get(ptr))*(uint32_t)A) >> 8 ) + (uint32_t)Channels::px<bpp>::g.get(ptr),
				  ((((uint32_t)B - (uint32_t)Channels::px<bpp>::b.get(ptr))*(uint32_t)A) >> 8 ) + (uint32_t)Channels::px<bpp>::b.get(ptr));
}


template<int bpp, int incrementPtr>
STRONG_INLINE void ColorPutter<bpp, incrementPtr>::PutColor(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B)
{
	static_assert(incrementPtr >= -1 && incrementPtr <= +1, "Invalid incrementPtr value!");

	if (incrementPtr < 0)
		ptr -= bpp;

	Channels::px<bpp>::r.set(ptr, R);
	Channels::px<bpp>::g.set(ptr, G);
	Channels::px<bpp>::b.set(ptr, B);
	Channels::px<bpp>::a.set(ptr, 255);

	if (incrementPtr > 0)
		ptr += bpp;

}

template<int bpp, int incrementPtr>
STRONG_INLINE void ColorPutter<bpp, incrementPtr>::PutColorRow(uint8_t *&ptr, const SDL_Color & Color, size_t count)
{
	if (count)
	{
		uint8_t *pixel = ptr;
		PutColor(ptr, Color.r, Color.g, Color.b);

		for (size_t i=0; i<count-1; i++)
		{
			memcpy(ptr, pixel, bpp);
			ptr += bpp * incrementPtr;
		}
	}
}

template <int incrementPtr>
STRONG_INLINE void ColorPutter<2, incrementPtr>::PutColor(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B)
{
	if(incrementPtr == -1)
		ptr -= 2;

	auto * const px = (uint16_t *)ptr;
	*px = (B>>3) + ((G>>2) << 5) + ((R>>3) << 11); //drop least significant bits of 24 bpp encoded color

	if(incrementPtr == 1)
		ptr += 2; //bpp
}

template <int incrementPtr>
STRONG_INLINE void ColorPutter<2, incrementPtr>::PutColorAlphaSwitch(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A)
{
	switch (A)
	{
	case 0:
		ptr += 2 * incrementPtr;
		return;
	case 255:
		PutColor(ptr, R, G, B);
		return;
	default:
		PutColor(ptr, R, G, B, A);
		return;
	}
}

template <int incrementPtr>
STRONG_INLINE void ColorPutter<2, incrementPtr>::PutColor(uint8_t *&ptr, const uint8_t & R, const uint8_t & G, const uint8_t & B, const uint8_t & A)
{
	const int rbit = 5, gbit = 6, bbit = 5; //bits per color
	const int rmask = 0xF800, gmask = 0x7E0, bmask = 0x1F;
	const int rshift = 11, gshift = 5, bshift = 0;

	const uint8_t r5 = (*((uint16_t *)ptr) & rmask) >> rshift,
		b5 = (*((uint16_t *)ptr) & bmask) >> bshift,
		g5 = (*((uint16_t *)ptr) & gmask) >> gshift;

	const uint32_t r8 = (r5 << (8 - rbit)) | (r5 >> (2*rbit - 8)),
		g8 = (g5 << (8 - gbit)) | (g5 >> (2*gbit - 8)),
		b8 = (b5 << (8 - bbit)) | (b5 >> (2*bbit - 8));

	PutColor(ptr,
		(((R-r8)*A) >> 8) + r8,
		(((G-g8)*A) >> 8) + g8,
		(((B-b8)*A) >> 8) + b8);
}

template <int incrementPtr>
STRONG_INLINE void ColorPutter<2, incrementPtr>::PutColorAlpha(uint8_t *&ptr, const SDL_Color & Color)
{
	PutColor(ptr, Color.r, Color.g, Color.b, Color.a);
}

template <int incrementPtr>
STRONG_INLINE void ColorPutter<2, incrementPtr>::PutColor(uint8_t *&ptr, const SDL_Color & Color)
{
	PutColor(ptr, Color.r, Color.g, Color.b);
}

template <int incrementPtr>
STRONG_INLINE void ColorPutter<2, incrementPtr>::PutColorRow(uint8_t *&ptr, const SDL_Color & Color, size_t count)
{
	//drop least significant bits of 24 bpp encoded color
	uint16_t pixel = (Color.b>>3) + ((Color.g>>2) << 5) + ((Color.r>>3) << 11);

	for (size_t i=0; i<count; i++)
	{
		memcpy(ptr, &pixel, 2);
		if(incrementPtr == -1)
			ptr -= 2;
		if(incrementPtr == 1)
			ptr += 2;
	}
}
