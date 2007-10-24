#ifndef MAPHANDLER_H
#define MAPHANDLER_H

#include "hch\CAmbarCendamo.h"
#include "hch\CSemiDefHandler.h"
#include "CGameInfo.h"
#include "hch\CDefHandler.h"
#include <boost/logic/tribool.hpp>
#include "hch\CObjectHandler.h"
#include <list>
const int Woff = 12; //width of map's frame
const int Hoff = 8; 

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

	std::vector < std::pair<CObjectInstance*,std::pair<SDL_Rect, std::vector<std::list<int3>>>> > objects; //poiters to objects being on this tile with rects to be easier to blit this tile on screen
	std::vector <CObjectInstance*> visitableObjects; //pointers to objects hero is visiting being on this tile

};

//pathfinder
//	map<int,int> iDTerenu=>koszt_pola
//	map<int,int> IDdrogi=>koszt_drogi
template <typename T> class PseudoV
{
public:
	int offset;
	std::vector<T> inver;
	inline T & operator[](int n)
	{
		return inver[n+offset];
	}
	void resize(int rest,int Offset)
	{
		inver.resize(Offset*2+rest);
		offset=Offset;
	}
	int size() const
	{
		return inver.size();
	}
};
class CMapHandler
{
public:
	PseudoV< PseudoV< PseudoV<TerrainTile2> > > ttiles;
	int3 sizes;
	CAmbarCendamo * reader;

	CDefHandler * fullHide;
	CDefHandler * partialHide;

	PseudoV< PseudoV< PseudoV<unsigned char> > > visibility; //true means that pointed place is visible //not used now
	//std::vector< std::vector<char> > undVisibility; //true means that pointed place is visible
	std::vector<CDefHandler *> roadDefs;
	std::vector<CDefHandler *> staticRiverDefs;


	char & visAccess(int x, int y);
	char & undVisAccess(int x, int y);
	SDL_Surface mirrorImage(SDL_Surface *src); //what is this??
	SDL_Surface * getVisBitmap(int x, int y, PseudoV< PseudoV< PseudoV<unsigned char> > > & visibilityMap, int lvl);

	int getCost(int3 & a, int3 & b, const CHeroInstance * hero);
	std::vector< std::string > getObjDescriptions(int3 pos); //returns desriptions of objects blocking given position
	void init();
	SDL_Surface * terrainRect(int x, int y, int dx, int dy, int level=0, unsigned char anim=0, PseudoV< PseudoV< PseudoV<unsigned char> > > & visibilityMap = CGI->mh->visibility);
	SDL_Surface * terrBitmap(int x, int y);
	SDL_Surface * undTerrBitmap(int x, int y);

};

#endif //MAPHANDLER_H