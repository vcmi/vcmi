#ifndef __CBITMAPHANDLER_H__
#define __CBITMAPHANDLER_H__


#include "../global.h"
struct SDL_Surface;
class CLodHandler;

/*
 * CBitmapHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

enum Epcxformat {PCX8B, PCX24B};

/// Struct which stands for a simple rgba palette
struct BMPPalette
{
	unsigned char R,G,B,F;
};

/// Class which converts pcx to bmp images
class CPCXConv
{	
public:
	unsigned char * pcx, *bmp;
	int pcxs, bmps;
	void fromFile(std::string path);
	void saveBMP(std::string path) const;
	void openPCX(char * PCX, int len);
	SDL_Surface * getSurface() const; //for standard H3 PCX
	//SDL_Surface * getSurfaceZ(); //for ZSoft PCX
	CPCXConv() //c-tor
	: pcx(NULL), bmp(NULL), pcxs(0), bmps(0)
	{}
	~CPCXConv() //d-tor
	{
		if (pcxs) delete[] pcx;
		if (bmps) delete[] bmp;
	}
};

namespace BitmapHandler
{
	//Load file from specific LOD
	SDL_Surface * loadBitmapFromLod(CLodHandler *lod, std::string fname, bool setKey=true);
	//Load file from any LODs
	SDL_Surface * loadBitmap(std::string fname, bool setKey=true);
};

#endif // __CBITMAPHANDLER_H__
