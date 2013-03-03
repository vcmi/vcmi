#include "StdInc.h"

#include "Images.h"
#include "FilesHeaders.h"


namespace Gfx
{
#define LE(x) SDL_SwapLE32(x)

/*********** H3 PCX image format ***********/

CImage * CImage::makeFromPCX(const SH3PcxFile& pcx, size_t fileSize)
{
	ui32 imgSize = LE(pcx.size);
	ui32 width = LE(pcx.width);
	ui32 height = LE(pcx.height);

	if (imgSize == width*height)
	{
		if (H3PCX_HEADER_SIZE + RGB_PALETTE_SIZE + imgSize > fileSize) return nullptr;

		CPaletteRGBA* ppal = new CPaletteRGBA(pcx.palette(fileSize), 0);
		return new CPalettedBitmap(width, height, *ppal, pcx.data);
	}
	if (imgSize == width*height*3)
	{
		if (H3PCX_HEADER_SIZE + imgSize > fileSize) return nullptr;

		return new CBitmap32(width, height, (ColorRGB*)pcx.data);
	}

	return nullptr;
}


/*********** DEF animation frame ***********/

CImage* CImage::makeFromDEF(const struct SH3DefFile& def, size_t fileSize)
{
	CPaletteRGBA* ppal = new CPaletteRGBA(def.palette, def.type == LE(71) ? 1 : 10);

	CPalettedBitmap* img = makeFromDEFSprite(def.getSprite(def.firstBlock.offsets()[0]), *ppal);

	if (img == nullptr) ppal->Unlink();

	return img;
}


CPalettedBitmap* CImage::makeFromDEFSprite(const SH3DefSprite& spr, CPaletteRGBA& pal)
{
	ui32 format = LE(spr.format);
	if (format > 3) return nullptr;

	ui32 fullWidth = LE(spr.fullWidth);
	ui32 fullHeight = LE(spr.fullHeight);
	ui32 intWidth = LE(spr.width);
	ui32 intHeight = LE(spr.height);
	ui32 leftMargin = LE(spr.leftMargin);
	ui32 topMargin = LE(spr.topMargin);

	bool noMargins =  (leftMargin == 0 && topMargin == 0 && fullWidth == intWidth && fullHeight == intHeight);

	if (format == 0)
	{
		if (noMargins)
		{
			return new CPalettedBitmap(fullWidth, fullHeight, pal, spr.data); 
		}
		return new CPalBitmapWithMargin(fullWidth, fullHeight, leftMargin, topMargin, intWidth, intHeight, pal, spr.data);
	}

	if (noMargins)
	{
		return new CPalettedBitmap(fullWidth, fullHeight, pal, spr.data, format); 
	}
	return new CPalBitmapWithMargin(fullWidth, fullHeight, leftMargin, topMargin, intWidth, intHeight, pal, spr.data, format);
}


}
