#include "StdInc.h"

#include "../lib/filesystem/Filesystem.h"
#include "SDL.h"
#include "SDL_image.h"
#include "CBitmapHandler.h"
#include "CDefHandler.h"
#include "gui/SDL_Extensions.h"
#include "../lib/vcmi_endian.h"

/*
 * CBitmapHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

bool isPCX(const ui8 *header)//check whether file can be PCX according to header
{
	int fSize  = read_le_u32(header + 0);
	int width  = read_le_u32(header + 4);
	int height = read_le_u32(header + 8);
	return fSize == width*height || fSize == width*height*3;
}

enum Epcxformat
{
	PCX8B,
	PCX24B
};

SDL_Surface * BitmapHandler::loadH3PCX(ui8 * pcx, size_t size)
{
	SDL_Surface * ret;

	Epcxformat format;
	int it=0;

	ui32 fSize = read_le_u32(pcx + it); it+=4;
	ui32 width = read_le_u32(pcx + it); it+=4;
	ui32 height = read_le_u32(pcx + it); it+=4;

	if (fSize==width*height*3)
		format=PCX24B;
	else if (fSize==width*height)
		format=PCX8B;
	else
		return nullptr;

	if (format==PCX8B)
	{
		ret = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 8, 0, 0, 0, 0);

		it = 0xC;
		for (int i=0; i<height; i++)
		{
			memcpy((char*)ret->pixels + ret->pitch * i, pcx + it, width);
			it+= width;
		}

		//palette - last 256*3 bytes
		it = size-256*3;
		for (int i=0;i<256;i++)
		{
			SDL_Color tp;
			tp.r = pcx[it++];
			tp.g = pcx[it++];
			tp.b = pcx[it++];
			tp.a = SDL_ALPHA_OPAQUE;
			ret->format->palette->colors[i] = tp;
		}
	}
	else
	{
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
		int bmask = 0xff0000;
		int gmask = 0x00ff00;
		int rmask = 0x0000ff;
#else
		int bmask = 0x0000ff;
		int gmask = 0x00ff00;
		int rmask = 0xff0000;
#endif
		ret = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 24, rmask, gmask, bmask, 0);

		//it == 0xC;
		for (int i=0; i<height; i++)
		{
			memcpy((char*)ret->pixels + ret->pitch * i, pcx + it, width*3);
			it+= width*3;
		}

	}
	return ret;
}

SDL_Surface * BitmapHandler::loadBitmapFromDir(std::string path, std::string fname, bool setKey)
{
	if(!fname.size())
	{
		logGlobal->warnStream() << "Call to loadBitmap with void fname!";
		return nullptr;
	}
	if (!CResourceHandler::get()->existsResource(ResourceID(path + fname, EResType::IMAGE)))
	{
		return nullptr;
	}

	SDL_Surface * ret=nullptr;

	auto readFile = CResourceHandler::get()->load(ResourceID(path + fname, EResType::IMAGE))->readAll();

	if (isPCX(readFile.first.get()))
	{//H3-style PCX
		ret = loadH3PCX(readFile.first.get(), readFile.second);
		if (ret)
		{
			if(ret->format->BytesPerPixel == 1  &&  setKey)
			{
				CSDL_Ext::setColorKey(ret,ret->format->palette->colors[0]);
			}
		}
		else
		{
			logGlobal->errorStream()<<"Failed to open "<<fname<<" as H3 PCX!";
			return nullptr;
		}
	}
	else
	{ //loading via SDL_Image
		ret = IMG_Load_RW(
		          //create SDL_RW with our data (will be deleted by SDL)
		          SDL_RWFromConstMem((void*)readFile.first.get(), readFile.second),
		          1); // mark it for auto-deleting
		if (ret)
		{
			if (ret->format->palette)
			{
				//set correct value for alpha\unused channel
				for (int i=0; i < ret->format->palette->ncolors; i++)
					ret->format->palette->colors[i].a = SDL_ALPHA_OPAQUE;
			}
		}
		else
		{
			logGlobal->errorStream() << "Failed to open " << fname << " via SDL_Image";
			logGlobal->errorStream() << "Reason: " << IMG_GetError();
			return nullptr;
		}
	}

	// When modifying anything here please check two use cases:
	// 1) Vampire mansion in Necropolis (not 1st color is transparent)
	// 2) Battle background when fighting on grass/dirt, topmost sky part (NO transparent color)
	// 3) New objects that may use 24-bit images for icons (e.g. witchking arts)
	if (ret->format->palette)
	{
		CSDL_Ext::setDefaultColorKeyPresize(ret);
	}
	else // always set
	{
		CSDL_Ext::setDefaultColorKey(ret);
	}
	return ret;
}

SDL_Surface * BitmapHandler::loadBitmap(std::string fname, bool setKey)
{
	SDL_Surface *bitmap;

	if (!(bitmap = loadBitmapFromDir("DATA/", fname, setKey)) &&
		!(bitmap = loadBitmapFromDir("SPRITES/", fname, setKey)))
	{
		logGlobal->errorStream() << "Error: Failed to find file " << fname;
	}

	return bitmap;
}
