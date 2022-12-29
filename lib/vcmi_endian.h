/*
 * vcmi_endian.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

//FIXME:library file depends on SDL - make cause troubles
#include <SDL_endian.h>

VCMI_LIB_NAMESPACE_BEGIN

/* Reading values from memory.  
 *
 * read_le_u16, read_le_u32 : read a little endian value from
 *    memory. On big endian machines, the value will be byteswapped.
 */

#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)

#if defined(_MSC_VER)
#define PACKED_STRUCT_BEGIN __pragma( pack(push, 1) )
#define PACKED_STRUCT_END   __pragma( pack(pop) )
#else
#define PACKED_STRUCT_BEGIN
#define PACKED_STRUCT_END __attribute__(( packed ))
#endif

PACKED_STRUCT_BEGIN struct unaligned_Uint16 { ui16 val; } PACKED_STRUCT_END;
PACKED_STRUCT_BEGIN struct unaligned_Uint32 { ui32 val; } PACKED_STRUCT_END;

static inline ui16 read_unaligned_u16(const void *p)
{
	const struct unaligned_Uint16 *v = reinterpret_cast<const struct unaligned_Uint16 *>(p);
	return v->val;
}

static inline ui32 read_unaligned_u32(const void *p)
{
	const struct unaligned_Uint32 *v = reinterpret_cast<const struct unaligned_Uint32 *>(p);
	return v->val;
}

#define read_le_u16(p) (SDL_SwapLE16(read_unaligned_u16(p)))
#define read_le_u32(p) (SDL_SwapLE32(read_unaligned_u32(p)))

#else

#warning UB: unaligned memory access

#define read_le_u16(p) (SDL_SwapLE16(* reinterpret_cast<const ui16 *>(p)))
#define read_le_u32(p) (SDL_SwapLE32(* reinterpret_cast<const ui32 *>(p)))

#define PACKED_STRUCT_BEGIN
#define PACKED_STRUCT_END

#endif

static inline char readChar(const ui8 * buffer, int & i)
{
    return buffer[i++];
}

static inline std::string readString(const ui8 * buffer, int & i)
{
    int len = read_le_u32(buffer + i);
    i += 4;
	assert(len >= 0 && len <= 500000); //not too long
    std::string ret;
    ret.reserve(len);
    for(int gg = 0; gg < len; ++gg)
	{
        ret += buffer[i++];
	}
	return ret;
}

VCMI_LIB_NAMESPACE_END
