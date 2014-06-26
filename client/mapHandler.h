#pragma once


#include "../lib/int3.h"
#include "SDL.h"

/*
 * mapHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGObjectInstance;
class CGHeroInstance;
class CMap;
class CGDefInfo;
class CGObjectInstance;
class CDefHandler;
struct TerrainTile;
struct SDL_Surface;
//struct SDL_Rect;
class CDefEssential;

struct TerrainTile2
{
	SDL_Surface * terbitmap; //bitmap of terrain

	std::vector < std::pair<const CGObjectInstance*,SDL_Rect> > objects; //pointers to objects being on this tile with rects to be easier to blit this tile on screen
	TerrainTile2();
};

template <typename T> class PseudoV
{
public:
	PseudoV() : offset(0) { }
	PseudoV(std::vector<T> &src, int rest, int before, int after, const T& fill) : offset(before)
	{
		inver.resize(before + rest + after);
		for(int i=0; i<before;i++)
			inver[i] = fill;
		for(int i=0;i<src.size();i++)
			inver[offset+i] = src[i];
		for(int i=src.size(); i<src.size()+after;i++)
			inver[offset+i] = fill;
	}
	inline T & operator[](const int & n)
	{
		return inver[n+offset];
	}
	inline const T & operator[](const int & n) const
	{
		return inver[n+offset];
	}
	void resize(int rest, int before, int after)
	{
		inver.resize(before + rest + after);
		offset=before;
	}
	int size() const
	{
		return inver.size();
	}

private:
	int offset;
	std::vector<T> inver;
};

class CMapHandler
{
public:
	PseudoV< PseudoV< PseudoV<TerrainTile2> > > ttiles; //informations about map tiles
	int3 sizes; //map size (x = width, y = height, z = number of levels)
	const CMap * map;

	// Max number of tiles that will fit in the map screen. Tiles
	// can be partial on each edges.
	int tilesW;
	int tilesH;

	// size of each side of the frame around the whole map, in tiles
	int frameH;
	int frameW;

	// Coord in pixels of the top left corner of the top left tile to
	// draw. Values range is [-31..0]. A negative value
	// implies that part of the tile won't be displayed.
	int offsetX;
	int offsetY;

	//std::set<int> usedHeroes;

	std::vector<std::vector<SDL_Surface *> > terrainGraphics; // [terrain id] [view type] [rotation type]
	std::vector<CDefEssential *> roadDefs;
	std::vector<CDefEssential *> staticRiverDefs;

	std::vector<std::vector<std::vector<ui8> > > hideBitmap; //specifies number of graphic that should be used to fully hide a tile

	mutable std::map<const CGObjectInstance*, ui8> animationPhase;

	CMapHandler(); //c-tor
	~CMapHandler(); //d-tor

	std::pair<SDL_Surface *, bool> getVisBitmap(const int3 & pos, const std::vector< std::vector< std::vector<ui8> > > & visibilityMap) const; //returns appropriate bitmap and info if alpha blitting is necessary
	ui8 getPhaseShift(const CGObjectInstance *object) const;

	void getTerrainDescr(const int3 &pos, std::string & out, bool terName); //if tername == false => empty string when tile is clear
	CGObjectInstance * createObject(int id, int subid, int3 pos, int owner=254); //creates a new object with a certain id and subid
	bool printObject(const CGObjectInstance * obj); //puts appropriate things to ttiles, so obj will be visible on map
	bool hideObject(const CGObjectInstance * obj); //removes appropriate things from ttiles, so obj will be no longer visible on map (but still will exist)
	bool removeObject(CGObjectInstance * obj); //removes object from each place in VCMI (I hope)
	void init();
	void calculateBlockedPos();
	void initObjectRects();
	void borderAndTerrainBitmapInit();
	void roadsRiverTerrainInit();
	void prepareFOWDefs();

	void terrainRect(int3 top_tile, ui8 anim, const std::vector< std::vector< std::vector<ui8> > > * visibilityMap, bool otherHeroAnim, ui8 heroAnim, SDL_Surface * extSurf, const SDL_Rect * extRect, int moveX, int moveY, bool puzzleMode, int3 grailPosRel) const;
	void updateWater();
	ui8 getHeroFrameNum(ui8 dir, bool isMoving) const; //terrainRect helper function
	void validateRectTerr(SDL_Rect * val, const SDL_Rect * ext); //terrainRect helper
	static ui8 getDir(const int3 & a, const int3 & b);  //returns direction number in range 0 - 7 (0 is left top, clockwise) [direction: form a to b]

	static bool compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b);

};
