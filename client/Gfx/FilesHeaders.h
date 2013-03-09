#pragma once

#include <SDL_endian.h>
#include "CPaletteRGBA.h"


namespace Gfx
{
#define RGB_PALETTE_SIZE 0x300
#define H3PCX_HEADER_SIZE sizeof(SH3PcxFile)
#define H3DEF_HEADER_SIZE sizeof(SH3DefFile)

#define SELF_ADDR reinterpret_cast<const ui8*>(this)

#ifdef _MSC_VER
#pragma pack(1)
#pragma warning(disable : 4200)
#endif

struct SH3PcxFile
{
	ui32 size;
	ui32 width;
	ui32 height;
	ui8 data[];

	// palette = last 256*3 bytes of PCX
	inline const ColorRGB* palette(size_t sizeOfPcx) const {
		return (ColorRGB*) (SELF_ADDR + sizeOfPcx - RGB_PALETTE_SIZE);
	}
};


struct SH3DefSprite
{
	ui32 size;
	ui32 format;    /// format in which pixel data is stored
	ui32 fullWidth; /// full width and height of frame, including borders
	ui32 fullHeight;
	ui32 width;     /// width and height of pixel data, borders excluded
	ui32 height;
	si32 leftMargin;
	si32 topMargin;
	ui8 data[];
};


struct SH3DefBlock {
	ui32 id;
	ui32 entriesCount;
	ui32 unknown1;
	ui32 unknown2;
	char names[][13];	// [entriesCount][13] - array of frames names

	inline ua_ui32_ptr offsets() const {
		return (ua_ui32_ptr)(names + SDL_SwapLE32(entriesCount));
	}	// array of offsets of frames
};


struct SH3DefFile {
	ui32 type;
	ui32 width;
	ui32 height;
	ui32 totalBlocks;
	ColorRGB palette[256];

	// SDefHeader is followed by a series of SH3DefBlock
	SH3DefBlock firstBlock;

	// Number of entries (sprites) in first block
	inline ui32 fbEntrCount() const { return firstBlock.entriesCount; };

	inline SH3DefSprite& getSprite(ui32 offset) const {
		return *(SH3DefSprite*) (SELF_ADDR + offset);
	}
};

#ifdef _MSC_VER
#pragma warning(default : 4200)
#pragma pack()
#endif

#undef SELF_ADDR
}
