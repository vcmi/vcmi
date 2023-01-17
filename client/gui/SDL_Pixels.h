/*
 * SDL_Pixels.h, part of VCMI engine
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

// for accessing channels from pixel in format-independent way
//should be as fast as accessing via pointer at least for 3-4 bytes per pixel
namespace Channels
{
	// channel not present in this format
	struct channel_empty
	{
		static STRONG_INLINE void set(Uint8*, Uint8) {}
		static STRONG_INLINE Uint8 get(const Uint8 *) { return 255;}
	};

	// channel which uses whole pixel
	template<int offset>
	struct channel_pixel
	{
		static STRONG_INLINE void set(Uint8 *ptr, Uint8 value)
		{
			ptr[offset] = value;
		}

		static STRONG_INLINE Uint8 get(const Uint8 *ptr)
		{
			return ptr[offset];
		}
	};

	// channel which uses part of pixel
	template <int bits, int mask, int shift>
	struct channel_subpx
	{

		static void STRONG_INLINE set(Uint8 *ptr, Uint8 value)
		{
			Uint16 * const pixel = (Uint16*)ptr;
			Uint8 subpx = value >> (8 - bits);
			*pixel = (*pixel & ~mask) | ((subpx << shift) & mask );
		}

		static Uint8 STRONG_INLINE get(const Uint8 *ptr)
		{
			Uint8 subpx = (*((Uint16 *)ptr) & mask) >> shift;
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

#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)

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
	static STRONG_INLINE void PutColor(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B);
	static STRONG_INLINE void PutColor(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B, const Uint8 & A);
	static STRONG_INLINE void PutColorAlphaSwitch(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B, const Uint8 & A);
	static STRONG_INLINE void PutColor(Uint8 *&ptr, const SDL_Color & Color);
	static STRONG_INLINE void PutColorAlpha(Uint8 *&ptr, const SDL_Color & Color);
	static STRONG_INLINE void PutColorRow(Uint8 *&ptr, const SDL_Color & Color, size_t count);
};

template <int incrementPtr>
struct ColorPutter<2, incrementPtr>
{
	static STRONG_INLINE void PutColor(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B);
	static STRONG_INLINE void PutColor(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B, const Uint8 & A);
	static STRONG_INLINE void PutColorAlphaSwitch(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B, const Uint8 & A);
	static STRONG_INLINE void PutColor(Uint8 *&ptr, const SDL_Color & Color);
	static STRONG_INLINE void PutColorAlpha(Uint8 *&ptr, const SDL_Color & Color);
	static STRONG_INLINE void PutColorRow(Uint8 *&ptr, const SDL_Color & Color, size_t count);
};

template<int bpp, int incrementPtr>
STRONG_INLINE void ColorPutter<bpp, incrementPtr>::PutColorAlpha(Uint8 *&ptr, const SDL_Color & Color)
{
	PutColor(ptr, Color.r, Color.g, Color.b, Color.a);
}

template<int bpp, int incrementPtr>
STRONG_INLINE void ColorPutter<bpp, incrementPtr>::PutColor(Uint8 *&ptr, const SDL_Color & Color)
{
	PutColor(ptr, Color.r, Color.g, Color.b);
}

template<int bpp, int incrementPtr>
STRONG_INLINE void ColorPutter<bpp, incrementPtr>::PutColorAlphaSwitch(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B, const Uint8 & A)
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
				 ((Uint16)R + Channels::px<bpp>::r.get(ptr)) >> 1,
				 ((Uint16)G + Channels::px<bpp>::g.get(ptr)) >> 1,
				 ((Uint16)B + Channels::px<bpp>::b.get(ptr)) >> 1);
		return;
	default:
		PutColor(ptr, R, G, B, A);
		return;
	}
}

template<int bpp, int incrementPtr>
STRONG_INLINE void ColorPutter<bpp, incrementPtr>::PutColor(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B, const Uint8 & A)
{
	PutColor(ptr, ((((Uint32)R - (Uint32)Channels::px<bpp>::r.get(ptr))*(Uint32)A) >> 8 ) + (Uint32)Channels::px<bpp>::r.get(ptr),
				  ((((Uint32)G - (Uint32)Channels::px<bpp>::g.get(ptr))*(Uint32)A) >> 8 ) + (Uint32)Channels::px<bpp>::g.get(ptr),
				  ((((Uint32)B - (Uint32)Channels::px<bpp>::b.get(ptr))*(Uint32)A) >> 8 ) + (Uint32)Channels::px<bpp>::b.get(ptr));
}


template<int bpp, int incrementPtr>
STRONG_INLINE void ColorPutter<bpp, incrementPtr>::PutColor(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B)
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
STRONG_INLINE void ColorPutter<bpp, incrementPtr>::PutColorRow(Uint8 *&ptr, const SDL_Color & Color, size_t count)
{
	if (count)
	{
		Uint8 *pixel = ptr;
		PutColor(ptr, Color.r, Color.g, Color.b);

		for (size_t i=0; i<count-1; i++)
		{
			memcpy(ptr, pixel, bpp);
			ptr += bpp * incrementPtr;
		}
	}
}

template <int incrementPtr>
STRONG_INLINE void ColorPutter<2, incrementPtr>::PutColor(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B)
{
	if(incrementPtr == -1)
		ptr -= 2;

	Uint16 * const px = (Uint16*)ptr;
	*px = (B>>3) + ((G>>2) << 5) + ((R>>3) << 11); //drop least significant bits of 24 bpp encoded color

	if(incrementPtr == 1)
		ptr += 2; //bpp
}

template <int incrementPtr>
STRONG_INLINE void ColorPutter<2, incrementPtr>::PutColorAlphaSwitch(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B, const Uint8 & A)
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
STRONG_INLINE void ColorPutter<2, incrementPtr>::PutColor(Uint8 *&ptr, const Uint8 & R, const Uint8 & G, const Uint8 & B, const Uint8 & A)
{
	const int rbit = 5, gbit = 6, bbit = 5; //bits per color
	const int rmask = 0xF800, gmask = 0x7E0, bmask = 0x1F;
	const int rshift = 11, gshift = 5, bshift = 0;

	const Uint8 r5 = (*((Uint16 *)ptr) & rmask) >> rshift,
		b5 = (*((Uint16 *)ptr) & bmask) >> bshift,
		g5 = (*((Uint16 *)ptr) & gmask) >> gshift;

	const Uint32 r8 = (r5 << (8 - rbit)) | (r5 >> (2*rbit - 8)),
		g8 = (g5 << (8 - gbit)) | (g5 >> (2*gbit - 8)),
		b8 = (b5 << (8 - bbit)) | (b5 >> (2*bbit - 8));

	PutColor(ptr,
		(((R-r8)*A) >> 8) + r8,
		(((G-g8)*A) >> 8) + g8,
		(((B-b8)*A) >> 8) + b8);
}

template <int incrementPtr>
STRONG_INLINE void ColorPutter<2, incrementPtr>::PutColorAlpha(Uint8 *&ptr, const SDL_Color & Color)
{
	PutColor(ptr, Color.r, Color.g, Color.b, Color.a);
}

template <int incrementPtr>
STRONG_INLINE void ColorPutter<2, incrementPtr>::PutColor(Uint8 *&ptr, const SDL_Color & Color)
{
	PutColor(ptr, Color.r, Color.g, Color.b);
}

template <int incrementPtr>
STRONG_INLINE void ColorPutter<2, incrementPtr>::PutColorRow(Uint8 *&ptr, const SDL_Color & Color, size_t count)
{
	//drop least significant bits of 24 bpp encoded color
	Uint16 pixel = (Color.b>>3) + ((Color.g>>2) << 5) + ((Color.r>>3) << 11);

	for (size_t i=0; i<count; i++)
	{
		memcpy(ptr, &pixel, 2);
		if(incrementPtr == -1)
			ptr -= 2;
		if(incrementPtr == 1)
			ptr += 2;
	}
}
