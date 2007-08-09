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
	int3 pos;
	EterrainType typ;

	Eroad malle;
	unsigned char roaddir;

	Eriver nuine;
	unsigned char  rivdir;

	std::vector<SDL_Surface *> terbitmap; //frames of animation
	std::vector<SDL_Surface *> rivbitmap; //frames of animation
	std::vector<SDL_Surface *> roadbitmap; //frames of animation

	boost::logic::tribool state; //false = free; true = blocked; middle = visitable

	std::vector < std::pair<CObjectInstance*,SDL_Rect> > objects;
	std::vector <CObjectInstance*> visitableObjects;

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
	void init();
};

#endif //MAPHANDLER_H