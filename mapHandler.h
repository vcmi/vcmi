#ifndef MAPHANDLER_H
#define MAPHANDLER_H

#include "CAmbarCendamo.h"
#include "CSemiDefHandler.h"
#include "CGameInfo.h"
#include "CDefHandler.h"

const int Woff = 4; //width of map's frame
const int Hoff = 4; 

struct ObjSorter
{
	SDL_Surface * bitmap;
	int xpos, ypos;
	bool operator<(const ObjSorter & por) const;
};

class CMapHandler
{
public:
	CAmbarCendamo * reader;
	SDL_Surface *** terrainBitmap;
	SDL_Surface *** undTerrainBitmap; // used only if there is underground level
	SDL_Surface * terrainRect(int x, int y, int dx, int dy, int level=0, unsigned char anim=0);
	SDL_Surface * terrBitmap(int x, int y);
	SDL_Surface * undTerrBitmap(int x, int y);
	CDefHandler * fullHide;
	CDefHandler * partialHide;

	std::vector< std::vector<char> > visibility; //true means that pointed place is visible
	std::vector< std::vector<char> > undVisibility; //true means that pointed place is visible
	std::vector<CDefHandler *> roadDefs;
	std::vector<CDefHandler *> staticRiverDefs;
	SDL_Surface *** roadBitmaps;
	SDL_Surface *** undRoadBitmaps;
	SDL_Surface *** staticRiverBitmaps;
	SDL_Surface *** undStaticRiverBitmaps;
	char & visAccess(int x, int y);
	char & undVisAccess(int x, int y);
	SDL_Surface mirrorImage(SDL_Surface *src); //what is this??
	SDL_Surface * getVisBitmap(int x, int y, std::vector< std::vector<char> > & visibility);
	void init();
};

#endif //MAPHANDLER_H