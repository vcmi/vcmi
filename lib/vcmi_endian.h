#include <SDL_endian.h>

/* Reading values from memory.  
 *
 * read_le_u16, read_le_u32 : read a little endian value from
 *    memory. On big endian machines, the value will be byteswapped.
 */

#if defined(linux) && defined(sparc) 
/* SPARC does not support unaligned memory access. Let gcc know when
 * to emit the right code. */
struct unaligned_Uint16 { ui16 val __attribute__(( packed )); };
struct unaligned_Uint32 { ui32 val __attribute__(( packed )); };

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
#define read_le_u16(p) (SDL_SwapLE16(* reinterpret_cast<const ui16 *>(p)))
#define read_le_u32(p) (SDL_SwapLE32(* reinterpret_cast<const ui32 *>(p)))
#endif
