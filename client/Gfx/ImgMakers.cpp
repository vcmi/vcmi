#include "StdInc.h"

#include "SDL_image.h"
#include "Images.h"
#include "FilesHeaders.h"


namespace Gfx
{

/*********** Images formats supported by SDL ***********/

CImage * CImage::makeBySDL(void* data, size_t fileSize, const char* fileExt)
{
	SDL_Surface* ret = IMG_LoadTyped_RW(
		        //create SDL_RW with our data (will be deleted by SDL)
		        SDL_RWFromConstMem(data, fileSize),
		        1, // mark it for auto-deleting
		        const_cast<char*>(fileExt)); //pass extension without dot (+1 character)
	if (ret)
	{
		if (ret->format->palette)
		{
			//set correct value for alpha\unused channel
			for (int i=0; i< ret->format->palette->ncolors; i++)
				ret->format->palette->colors[i].unused = 255;

			CPaletteRGBA* pal = new CPaletteRGBA((ColorRGBA*)ret->format->palette->colors);
			return new CPalettedBitmap(ret->w, ret->h, *pal, (ui8*)ret->pixels);
		}

		return new CBitmap32(ret->w, ret->h, (ColorRGB*)ret->pixels);
	}
}


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


/*********** H3 DEF animation frame ***********/

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
