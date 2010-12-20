#ifndef __CDEFHANDLER_H__
#define __CDEFHANDLER_H__

#include "../global.h"

struct SDL_Surface;
struct BMPPalette;

/*
 * CDefHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct Cimage
{
	int groupNumber;
	std::string imName; //name without extension
	SDL_Surface * bitmap;
};

// Def entry in file. Integer fields are all little endian and will
// need to be converted.
struct SDefEntryBlock {
	ui32 unknown1;
	ui32 totalInBlock;
	ui32 unknown2;
	ui32 unknown3;
	unsigned char data[0];
};

// Def entry in file. Integer fields are all little endian and will
// need to be converted.
struct SDefEntry {
	ui32 DEFType;
	ui32 width;
	ui32 height;
	ui32 totalBlocks;

	struct {
		unsigned char R;
		unsigned char G;
		unsigned char B;
	} palette[256];

	// SDefEntry is followed by a series of SDefEntryBlock
	// This is commented out because VC++ doesn't accept C99 syntax.
	//struct SDefEntryBlock blocks[];
};

// Def entry in file. Integer fields are all little endian and will
// need to be converted.
struct SSpriteDef {
	ui32 prSize;
	ui32 defType2;
	ui32 FullWidth;
	ui32 FullHeight;
	ui32 SpriteWidth;
	ui32 SpriteHeight;
	ui32 LeftMargin;
	ui32 TopMargin;
};

class CDefEssential //DefHandler with images only
{
public:
	std::vector<Cimage> ourImages;
	~CDefEssential(); //d-tor
};

class CDefHandler
{
private:
	unsigned int DEFType;
	int length;
	//unsigned int * RWEntries;
	struct SEntry
	{
		std::string name;
		int offset;
		int group;
	} ;
	std::vector<SEntry> SEntries ;

public:
	int width, height; //width and height
	std::string defName;
	std::vector<Cimage> ourImages;
	bool notFreeImgs;

	CDefHandler(); //c-tor
	~CDefHandler(); //d-tor
	SDL_Surface * getSprite (int SIndex, const unsigned char * FDef, const BMPPalette * palette) const; //saves picture with given number to "testtt.bmp"
	static void expand(unsigned char N,unsigned char & BL, unsigned char & BR);
	void openFromMemory(unsigned char * table, const std::string & name);
	CDefEssential * essentialize();

	static CDefHandler * giveDef(const std::string & defName);
	static CDefEssential * giveDefEss(const std::string & defName);
};



#endif // __CDEFHANDLER_H__
