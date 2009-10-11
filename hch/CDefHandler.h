#ifndef __CDEFHANDLER_H__
#define __CDEFHANDLER_H__
#include "../client/CBitmapHandler.h"
#include <SDL_stdinc.h>
struct SDL_Surface;
class CDefEssential;
class CLodHandler;

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
struct defEntryBlock {
	Uint32 unknown1;
	Uint32 totalInBlock;
	Uint32 unknown2;
	Uint32 unknown3;
	unsigned char data[0];
};

// Def entry in file. Integer fields are all little endian and will
// need to be converted.
struct defEntry {
	Uint32 DEFType;
	Uint32 width;
	Uint32 height;
	Uint32 totalBlocks;

	struct {
		unsigned char R;
		unsigned char G;
		unsigned char B;
	} palette[256];

	struct defEntryBlock blocks[0];
};

// Def entry in file. Integer fields are all little endian and will
// need to be converted.
struct spriteDef {
	Uint32 prSize;
	Uint32 defType2;
	Uint32 FullWidth;
	Uint32 FullHeight;
	Uint32 SpriteWidth;
	Uint32 SpriteHeight;
	Uint32 LeftMargin;
	Uint32 TopMargin;
};

class CDefHandler
{
private:
	unsigned int totalEntries, DEFType, totalBlocks;
	bool allowRepaint;
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
	std::string defName, curDir;
	std::vector<Cimage> ourImages;
	bool alphaTransformed;
	bool notFreeImgs;

	CDefHandler(); //c-tor
	~CDefHandler(); //d-tor
	static void print (std::ostream & stream, int nr, int bytcon);
	static int readNormalNr (int pos, int bytCon, const unsigned char * str=NULL, bool cyclic=false);
	static unsigned char *writeNormalNr (int nr, int bytCon);
	SDL_Surface * getSprite (int SIndex, unsigned char * FDef, BMPPalette * palette); //zapisuje klatke o zadanym numerze do "testtt.bmp"
	void openDef(std::string name);
	void expand(unsigned char N,unsigned char & BL, unsigned char & BR);
	void openFromMemory(unsigned char * table, std::string name);
	CDefEssential * essentialize();

	static CDefHandler * giveDef(std::string defName);
	static CDefEssential * giveDefEss(std::string defName);
};

class CDefEssential //DefHandler with images only
{
public:
	std::vector<Cimage> ourImages;
	~CDefEssential(); //d-tor
};


#endif // __CDEFHANDLER_H__
