#include "../stdafx.h"
#include "SDL.h"
#include "SDL_image.h"
#include "CBitmapHandler.h"
#include "../hch/CDefHandler.h"
#include "../hch/CLodHandler.h"
#include <sstream>
#include <boost/thread.hpp>

/*
 * CBitmapHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

boost::mutex bitmap_handler_mx;

extern DLL_EXPORT CLodHandler *bitmaph;

void CPCXConv::openPCX(char * PCX, int len)
{
	pcxs=len;
	pcx=(unsigned char*)PCX;
}
void CPCXConv::fromFile(std::string path)
{
	std::ifstream is;
	is.open(path.c_str(),std::ios::binary);
	is.seekg(0,std::ios::end); // to the end
	pcxs = is.tellg();  // read length
	is.seekg(0,std::ios::beg); // wracamy na poczatek
	pcx = new unsigned char[pcxs]; // allocate memory 
	is.read((char*)pcx, pcxs); // read map file to buffer
	is.close();
}

void CPCXConv::saveBMP(std::string path) const
{
	std::ofstream os;
	os.open(path.c_str(), std::ios::binary);
	os.write(reinterpret_cast<const char*>(bmp), bmps);
	os.close();
}

SDL_Surface * CPCXConv::getSurface() const
{
	SDL_Surface * ret;

	int width = -1, height = -1;
	Epcxformat format;
	int fSize,y;//,i; //TODO use me 'i'
	bool check1, check2;
	unsigned char add;
	int it=0;

	fSize = readNormalNr(pcx, it); it+=4;
	width = readNormalNr(pcx, it); it+=4;
	height = readNormalNr(pcx, it); it+=4;
	if (fSize==width*height*3)
		check1=true;
	else 
		check1=false;
	if (fSize==width*height)
		check2=true;
	else 
		check2=false;
	if (check1)
		format=PCX24B;
	else if (check2)
		format=PCX8B;
	else 
		return NULL;
	add = 4 - width%4;
	if (add==4)
		add=0;
	if (format==PCX8B)
	{
		ret = SDL_CreateRGBSurface(SDL_SWSURFACE, width+add, height, 8, 0, 0, 0, 0);
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
		ret = SDL_CreateRGBSurface(SDL_SWSURFACE, width+add, height, 24, rmask, gmask, bmask, 0);
	}
	if (format==PCX8B)
	{
		it = pcxs-256*3;
		for (int i=0;i<256;i++)
		{
			SDL_Color tp;
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			tp.b = pcx[it++];
			tp.g = pcx[it++];
			tp.r = pcx[it++];
#else
			tp.r = pcx[it++];
			tp.g = pcx[it++];
			tp.b = pcx[it++];
#endif
			tp.unused = 0;
			*(ret->format->palette->colors+i) = tp;
		}
		for (y=height; y>0; --y)
		{
			it = 0xC + (y-1)*width;
			memcpy((char*)ret->pixels + ret->pitch * (y-1), pcx + it, width);

			if (add>0)
			{
				memset((char*)ret->pixels + ret->pitch * (y-1) + width, 0, add);
			}
		}
	}
	else
	{
		for (y=height; y>0; y--)
		{
			it = 0xC + (y-1)*width*3;

			memcpy((char*)ret->pixels + ret->pitch * (y-1), pcx + it, width*3);

			if (add>0)
			{
				memset((char*)ret->pixels + ret->pitch * (y-1) + width*3, 0, add*3);
			}
		}
	}
	return ret;
}

bool isPCX(const unsigned char *header)//check whether file can be PCX according to 1st 12 bytes
{
	int fSize  = readNormalNr(header, 0);
	int width  = readNormalNr(header, 4);
	int height = readNormalNr(header, 8);
	return fSize == width*height || fSize == width*height*3;
}

SDL_Surface * BitmapHandler::loadBitmap(std::string fname, bool setKey)
{
	if(!fname.size())
	{
		tlog2 << "Call to loadBitmap with void fname!\n";
		return NULL;
	}
	SDL_Surface * ret=NULL;
	int size;
	unsigned char * file = bitmaph->giveFile(fname, FILE_GRAPHICS, &size);
	
	if (!file)
	{
		tlog2<<"Entry for file "<<fname<<" was not found"<<std::endl;
		return NULL;
	}
	
	if (isPCX(file))
	{//H3-style PCX
		CPCXConv cp;
		cp.openPCX((char*)file,size);
		ret = cp.getSurface();
		if (!ret)
			tlog1<<"Failed to open "<<fname<<" as H3 PCX!\n";
		if(ret->format->BytesPerPixel == 1  &&  setKey)
		{
			const SDL_Color &c = ret->format->palette->colors[0];
			SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format, c.r, c.g, c.b));
		}
	}
	else
	{ //loading via SDL_Image
		ret = IMG_Load_RW( SDL_RWFromMem((void*)file, size), 1);
		if (!ret)
			tlog1<<"Failed to open "<<fname<<" via SDL_Image\n";
	}
	return ret;
}
