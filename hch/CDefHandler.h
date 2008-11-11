#ifndef CDEFHANDLER_H
#define CDEFHANDLER_H
#include "../client/CBitmapHandler.h"
struct SDL_Surface;
class CDefEssential;
class CLodHandler;

struct Cimage
{
	int groupNumber;
	std::string imName; //name without extension
	SDL_Surface * bitmap;
};

class CDefHandler
{
private:
	int totalEntries, DEFType, totalBlocks, fullWidth, fullHeight;
	bool allowRepaint;
	int length;
	unsigned int * RWEntries;
	struct SEntry
	{
		std::string name;
		int offset;
		int group;
	} ;
	std::vector<SEntry> SEntries ;

public:
	static CLodHandler * Spriteh;
	std::string defName, curDir;
	std::vector<Cimage> ourImages;
	bool alphaTransformed;
	bool notFreeImgs;

	CDefHandler();
	~CDefHandler();
	static void print (std::ostream & stream, int nr, int bytcon);
	int readNormalNr (int pos, int bytCon, unsigned char * str=NULL, bool cyclic=false);
	static unsigned char *writeNormalNr (int nr, int bytCon);
	SDL_Surface * getSprite (int SIndex, unsigned char * FDef, BMPPalette * palette); //zapisuje klatke o zadanym numerze do "testtt.bmp"
	void openDef(std::string name);
	void expand(unsigned char N,unsigned char & BL, unsigned char & BR);
	void openFromMemory(unsigned char * table, std::string name);
	CDefEssential * essentialize();

	static CDefHandler * giveDef(std::string defName, CLodHandler * spriteh=NULL);
	static CDefEssential * giveDefEss(std::string defName, CLodHandler * spriteh=NULL);
};

class CDefEssential //DefHandler with images only
{
public:
	std::vector<Cimage> ourImages;
	~CDefEssential();
};

#endif // CDEFHANDLER_H
