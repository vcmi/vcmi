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
struct SDL_Rect;
class CDefEssential;

enum class EWorldViewIcon
{
	TOWN = 0,
	HERO,
	ARTIFACT,
	TELEPORT,
	GATE,
	MINE_WOOD,
	MINE_MERCURY,
	MINE_STONE,
	MINE_SULFUR,
	MINE_CRYSTAL,
	MINE_GEM,
	MINE_GOLD,
	RES_WOOD,
	RES_MERCURY,
	RES_STONE,
	RES_SULFUR,
	RES_CRYSTAL,
	RES_GEM,
	RES_GOLD,

};

struct TerrainTile2
{
	SDL_Surface * terbitmap; //bitmap of terrain

	std::vector < std::pair<const CGObjectInstance*,SDL_Rect> > objects; //pointers to objects being on this tile with rects to be easier to blit this tile on screen
	TerrainTile2();
};

struct MapDrawingInfo
{
	bool scaled;
	int3 &topTile; // top-left tile in viewport [in tiles]
	const std::vector< std::vector< std::vector<ui8> > > * visibilityMap;
	SDL_Rect * drawBounds; // map rect drawing bounds on screen
	CDefHandler * iconsDef; // holds overlay icons for world view mode
	float scale; // map scale for world view mode (only if scaled == true)

	bool otherheroAnim;
	ui8 anim;
	ui8 heroAnim;

	int3 movement; // used for smooth map movement

	bool puzzleMode;
	int3 grailPos; // location of grail for puzzle mode [in tiles]

	MapDrawingInfo(int3 &topTile_, const std::vector< std::vector< std::vector<ui8> > > * visibilityMap_, SDL_Rect * drawBounds_, CDefHandler * iconsDef_ = nullptr)
		: scaled(false),
		  topTile(topTile_),
		  visibilityMap(visibilityMap_),
		  drawBounds(drawBounds_),
		  iconsDef(iconsDef_),
		  scale(1.0f),
		  otherheroAnim(false),
		  anim(0u),
		  heroAnim(0u),
		  movement(int3()),
		  puzzleMode(false),
		  grailPos(int3())
	{}

	ui8 getHeroAnim() const { return otherheroAnim ? anim : heroAnim; }
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
	enum class EMapCacheType
	{
		TERRAIN, TERRAIN_CUSTOM, OBJECTS, ROADS, RIVERS, FOW, HEROES, HERO_FLAGS
	};

	/// temporarily caches rescaled sdl surfaces for map world view redrawing
	class CMapCache
	{
		std::map<EMapCacheType, std::map<intptr_t, SDL_Surface *>> data;
		float worldViewCachedScale;
	public:
		/// destroys all cached data (frees surfaces)
		void discardWorldViewCache();
		/// updates scale and determines if currently cached data is still valid
		void updateWorldViewScale(float scale);
		void removeFromWorldViewCache(EMapCacheType type, intptr_t key);
		/// asks for cached data; @returns cached surface or nullptr if data is not in cache
		SDL_Surface * requestWorldViewCache(EMapCacheType type, intptr_t key);
		/// asks for cached data; @returns cached data if found, new scaled surface otherwise
		SDL_Surface * requestWorldViewCacheOrCreate(EMapCacheType type, intptr_t key, SDL_Surface * fullSurface, float scale);
		SDL_Surface * cacheWorldViewEntry(EMapCacheType type, intptr_t key, SDL_Surface * entry);
		intptr_t genKey(intptr_t realPtr, ui8 mod);
	};


	class CMapBlitter
	{
	protected:
		CMapHandler * parent;
		int tileSize;
		int3 tileCount;
		int3 topTile;
		int3 initPos;
		int3 pos;
		int3 realPos;

		virtual void drawTileOverlay(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile2 & tile) = 0;
		virtual void drawNormalObject(SDL_Surface * targetSurf, const MapDrawingInfo * info, SDL_Surface * sourceSurf, SDL_Rect * sourceRect) = 0;
		virtual void drawHeroFlag(SDL_Surface * targetSurf, const MapDrawingInfo * info, SDL_Surface * sourceSurf, SDL_Rect * sourceRect, SDL_Rect * destRect, bool moving) = 0;
		virtual void drawHero(SDL_Surface * targetSurf, const MapDrawingInfo * info, SDL_Surface * sourceSurf, SDL_Rect * sourceRect, bool moving) = 0;
		void drawObjects(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile2 & tile);
		virtual void drawRoad(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile & tinfo, const TerrainTile * tinfoUpper) = 0;
		virtual void drawRiver(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile & tinfo) = 0;
		virtual void drawFow(SDL_Surface * targetSurf, const MapDrawingInfo * info) = 0;
		virtual void drawFrame(SDL_Surface * targetSurf, const MapDrawingInfo * info) = 0;
		virtual void drawTileTerrain(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile & tinfo, const TerrainTile2 & tile) = 0;
		virtual void init(const MapDrawingInfo * info) = 0;
		virtual SDL_Rect clip(SDL_Surface * targetSurf, const MapDrawingInfo * info) = 0;
	public:
		CMapBlitter(CMapHandler * p) : parent(p) {}
		virtual ~CMapBlitter(){}
		void blit(SDL_Surface * targetSurf, const MapDrawingInfo * const info);
	};

	class CMapNormalBlitter : public CMapBlitter
	{
	protected:
		void drawTileOverlay(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile2 & tile) {}
		void drawNormalObject(SDL_Surface * targetSurf, const MapDrawingInfo * info, SDL_Surface * sourceSurf, SDL_Rect * sourceRect);
		void drawHeroFlag(SDL_Surface * targetSurf, const MapDrawingInfo * info, SDL_Surface * sourceSurf, SDL_Rect * sourceRect, SDL_Rect * destRect, bool moving);
		void drawHero(SDL_Surface * targetSurf, const MapDrawingInfo * info, SDL_Surface * sourceSurf, SDL_Rect * sourceRect, bool moving);
		void drawRoad(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile & tinfo, const TerrainTile * tinfoUpper);
		void drawRiver(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile & tinfo);
		void drawFow(SDL_Surface * targetSurf, const MapDrawingInfo * info);
		void drawFrame(SDL_Surface * targetSurf, const MapDrawingInfo * info);
		void drawTileTerrain(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile & tinfo, const TerrainTile2 & tile);
		void init(const MapDrawingInfo * info);
		SDL_Rect clip(SDL_Surface * targetSurf, const MapDrawingInfo * info) override;
	public:
		CMapNormalBlitter(CMapHandler * parent);
		virtual ~CMapNormalBlitter(){}
	};

	class CMapWorldViewBlitter : public CMapBlitter
	{
	protected:
		int halfTargetTileSizeHigh;

		void drawTileOverlay(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile2 & tile);
		void drawNormalObject(SDL_Surface * targetSurf, const MapDrawingInfo * info, SDL_Surface * sourceSurf, SDL_Rect * sourceRect);
		void drawHeroFlag(SDL_Surface * targetSurf, const MapDrawingInfo * info, SDL_Surface * sourceSurf, SDL_Rect * sourceRect, SDL_Rect * destRect, bool moving);
		void drawHero(SDL_Surface * targetSurf, const MapDrawingInfo * info, SDL_Surface * sourceSurf, SDL_Rect * sourceRect, bool moving);
		void drawRoad(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile & tinfo, const TerrainTile * tinfoUpper);
		void drawRiver(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile & tinfo);
		void drawFow(SDL_Surface * targetSurf, const MapDrawingInfo * info);
		void drawFrame(SDL_Surface * targetSurf, const MapDrawingInfo * info) {}
		void drawTileTerrain(SDL_Surface * targetSurf, const MapDrawingInfo * info, const TerrainTile & tinfo, const TerrainTile2 & tile);
		void init(const MapDrawingInfo * info);
		SDL_Rect clip(SDL_Surface * targetSurf, const MapDrawingInfo * info) override;
	public:
		CMapWorldViewBlitter(CMapHandler * parent);
		virtual ~CMapWorldViewBlitter(){}
	};

//	class CPuzzleViewBlitter : public CMapNormalBlitter
//	{
//		void drawFow(SDL_Surface * targetSurf, const MapDrawingInfo * info) {} // skipping FoW in puzzle view
//	};

	CMapCache cache;
	CMapBlitter * normalBlitter;
	CMapBlitter * worldViewBlitter;

	void drawScaledRotatedElement(EMapCacheType type, SDL_Surface * baseSurf, SDL_Surface * targetSurf, ui8 rotation,
								  float scale, SDL_Rect * dstRect, SDL_Rect * srcRect = nullptr);
	void calculateWorldViewCameraPos(int targetTilesX, int targetTilesY, int3 &top_tile);
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

	void drawTerrainRectNew(SDL_Surface * targetSurface, const MapDrawingInfo * info);
	void updateWater();
	ui8 getHeroFrameNum(ui8 dir, bool isMoving) const; //terrainRect helper function
	void validateRectTerr(SDL_Rect * val, const SDL_Rect * ext); //terrainRect helper
	static ui8 getDir(const int3 & a, const int3 & b);  //returns direction number in range 0 - 7 (0 is left top, clockwise) [direction: form a to b]

	void discardWorldViewCache();

	static bool compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b);
};
