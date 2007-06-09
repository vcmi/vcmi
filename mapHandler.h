#include "CAmbarCendamo.h"
class mapHandler
{
public:
	CAmbarCendamo * reader;
	SDL_Surface *** terrainBitmap;
	SDL_Surface *** undTerrainBitmap; // used only if there is underground level
	SDL_Surface * terrainRect(int x, int y, int dx, int dy, int level=0);
	SDL_Surface mirrorImage(SDL_Surface *src);
	void init();
};