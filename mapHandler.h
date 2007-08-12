#ifndef MAPHANDLER_H
#define MAPHANDLER_H

#include "CAmbarCendamo.h"
#include "CSemiDefHandler.h"
#include "CGameInfo.h"
#include "CDefHandler.h"
#include <boost/logic/tribool.hpp>
const int Woff = 4; //width of map's frame
const int Hoff = 4; 

struct TerrainTile2
{
	int3 pos; //this tile's position
	EterrainType terType; //type of terrain tile

	Eroad malle; //type of road
	unsigned char roaddir; //type of road tile

	Eriver nuine; //type of river
	unsigned char  rivdir; //type of river tile

	std::vector<SDL_Surface *> terbitmap; //frames of terrain animation
	std::vector<SDL_Surface *> rivbitmap; //frames of river animation
	std::vector<SDL_Surface *> roadbitmap; //frames of road animation

	bool visitable; //false = not visitable; true = visitable
	bool blocked; //false = free; true = blocked;

	std::vector < std::pair<CObjectInstance*,SDL_Rect> > objects; //poiters to objects being on this tile with rects to be easier to blit this tile on screen
	std::vector <CObjectInstance*> visitableObjects; //pointers to objects hero is visiting being on this tile

};

//pathfinder
//	map<int,int> iDTerenu=>koszt_pola
//	map<int,int> IDdrogi=>koszt_drogi

class CMapHandler
{
public:
	std::vector< std::vector< std::vector<TerrainTile2> > > ttiles;
	CAmbarCendamo * reader;
	SDL_Surface * terrainRect(int x, int y, int dx, int dy, int level=0, unsigned char anim=0);
	SDL_Surface * terrBitmap(int x, int y);
	SDL_Surface * undTerrBitmap(int x, int y);
	CDefHandler * fullHide;
	CDefHandler * partialHide;

	std::vector< std::vector<char> > visibility; //true means that pointed place is visible
	std::vector< std::vector<char> > undVisibility; //true means that pointed place is visible
	std::vector<CDefHandler *> roadDefs;
	std::vector<CDefHandler *> staticRiverDefs;
	char & visAccess(int x, int y);
	char & undVisAccess(int x, int y);
	SDL_Surface mirrorImage(SDL_Surface *src); //what is this??
	SDL_Surface * getVisBitmap(int x, int y, std::vector< std::vector<char> > & visibility);

	int getCost(int3 & a, int3 & b, CHeroInstance * hero);
	void init();
};

#endif //MAPHANDLER_H