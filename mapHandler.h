#ifndef MAPHANDLER_H
#define MAPHANDLER_H
#include "global.h"
#include <SDL.h>
#include <list>
#include <set>
const int Woff = 12; //width of map's frame
const int Hoff = 8;

class CGObjectInstance;
class CGHeroInstance;
struct Mapa;
class CGDefInfo;
class CGObjectInstance;
class CDefHandler;
struct TerrainTile;

struct TerrainTile2
{
	int3 pos;
	const TerrainTile *tileInfo;
	std::vector<SDL_Surface *> terbitmap; //frames of terrain animation
	std::vector<SDL_Surface *> rivbitmap; //frames of river animation
	std::vector<SDL_Surface *> roadbitmap; //frames of road animation

	std::vector < std::pair<const CGObjectInstance*,SDL_Rect> > objects; //poiters to objects being on this tile with rects to be easier to blit this tile on screen
};

//pathfinder
//	map<int,int> iDTerenu=>koszt_pola
//	map<int,int> IDdrogi=>koszt_drogi
template <typename T> class PseudoV
{
public:
	int offset;
	std::vector<T> inver;
	PseudoV(){};
	PseudoV(std::vector<T> &src, int rest, int Offset, const T& fill)
	{
		inver.resize(Offset*2+rest);
		offset=Offset;
		for(int i=0; i<offset;i++)
			inver[i] = fill;
		for(int i=0;i<src.size();i++)
			inver[offset+i] = src[i];
		for(int i=src.size(); i<src.size()+offset;i++)
			inver[offset+i] = fill;
	}
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
	Mapa * map;
	std::set<int> usedHeroes;
	CDefHandler * fullHide;
	CDefHandler * partialHide;

	std::vector<CDefHandler *> roadDefs;
	std::vector<CDefHandler *> staticRiverDefs;
	std::vector<CDefHandler*> defs;

	std::map<std::string, CDefHandler*> loadedDefs; //pointers to loaded defs (key is filename, uppercase)

	std::vector<std::vector<std::vector<unsigned char> > > hideBitmap; //specifies number of graphic that should be used to fully hide a tile

	void loadDefs();
	SDL_Surface * getVisBitmap(int x, int y, std::vector< std::vector< std::vector<unsigned char> > > & visibilityMap, int lvl);

	int getCost(int3 & a, int3 & b, const CGHeroInstance * hero);
	std::vector< std::string > getObjDescriptions(int3 pos); //returns desriptions of objects blocking given position
	//std::vector< CGObjectInstance * > getVisitableObjs(int3 pos); //returns vector of visitable objects at certain position
	CGObjectInstance * createObject(int id, int subid, int3 pos, int owner=254); //creates a new object with a certain id and subid
	std::string getDefName(int id, int subid); //returns name of def for object with given id and subid
	bool printObject(CGObjectInstance * obj); //puts appropriate things to ttiles, so obj will be visible on map
	bool hideObject(CGObjectInstance * obj); //removes appropriate things from ttiles, so obj will be no longer visible on map (but still will exist)
	bool removeObject(CGObjectInstance * obj); //removes object from each place in VCMI (I hope)
	bool recalculateHideVisPos(int3& pos); //recalculates position for hidden / visitable positions
	bool recalculateHideVisPosUnderObj(CGObjectInstance * obj, bool withBorder = false); //recalculates position for hidden / visitable positions under given object
	void init();
	void calculateBlockedPos();
	void initObjectRects();
	void borderAndTerrainBitmapInit();
	void roadsRiverTerrainInit();
	void prepareFOWDefs();

	SDL_Surface * terrainRect(int x, int y, int dx, int dy, int level=0, unsigned char anim=0, std::vector< std::vector< std::vector<unsigned char> > > * visibilityMap = NULL, bool otherHeroAnim = false, unsigned char heroAnim = 0, SDL_Surface * extSurf = NULL, SDL_Rect * extRect = NULL); //if extSurf is specified, blit to it
	unsigned char getHeroFrameNum(const unsigned char & dir, const bool & isMoving) const; //terrainRect helper function
	void validateRectTerr(SDL_Rect * val, const SDL_Rect * ext); //terrainRect helper
	static unsigned char getDir(const int3 & a, const int3 & b); //returns direction number in range 0 - 7 (0 is left top, clockwise) [direction: form a to b]

};

#endif //MAPHANDLER_H
