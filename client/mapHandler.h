#pragma once


#include "../lib/int3.h"
#include "../lib/spells/ViewSpellInt.h"
#include "gui/Geometries.h"
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
class CGBoat;
class CMap;
class CGDefInfo;
class CDefHandler;
struct TerrainTile;
struct SDL_Surface;
struct SDL_Rect;
class CDefEssential;
class CFadeAnimation;
class PlayerColor;

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

enum class EMapObjectFadingType
{
	NONE,
	IN,
	OUT
};

enum class EMapAnimRedrawStatus
{
	OK,
	REDRAW_REQUESTED // map blitter requests quick redraw due to current animation
};

struct TerrainTileObject
{
	const CGObjectInstance *obj;
	SDL_Rect rect;
	int fadeAnimKey;
	
	TerrainTileObject(const CGObjectInstance *obj_, SDL_Rect rect_);
	~TerrainTileObject();
};

struct TerrainTile2
{
	SDL_Surface * terbitmap; //bitmap of terrain

	std::vector<TerrainTileObject> objects; //pointers to objects being on this tile with rects to be easier to blit this tile on screen
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
	
	const std::vector<ObjectPosInfo> * additionalIcons;
	
	bool showAllTerrain; //for expert viewEarth
	
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
		  grailPos(int3()),
		  additionalIcons(nullptr),
		  showAllTerrain(false)
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
		TERRAIN, TERRAIN_CUSTOM, OBJECTS, ROADS, RIVERS, FOW, HEROES, HERO_FLAGS, FRAME
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
	
	/// helper struct to pass around resolved bitmaps of an object; surfaces can be nullptr if object doesn't have bitmap of that type
	struct AnimBitmapHolder
	{
		SDL_Surface * objBitmap; // main object bitmap
		SDL_Surface * flagBitmap; // flag bitmap for the object (probably only for heroes and boats with heroes)
		bool isMoving; // indicates if the object is moving (again, heroes/boats only)
		
		AnimBitmapHolder(SDL_Surface * objBitmap_ = nullptr, SDL_Surface * flagBitmap_ = nullptr, bool moving = false)
			: objBitmap(objBitmap_),
			  flagBitmap(flagBitmap_),
			  isMoving(moving)
		{}
	};


	class CMapBlitter
	{		
	protected:
		const int FRAMES_PER_MOVE_ANIM_GROUP = 8;
		CMapHandler * parent; // ptr to enclosing map handler; generally for legacy reasons, probably could/should be refactored out of here
		int tileSize; // size of a tile drawn on map [in pixels]
		int halfTileSizeCeil; // half of the tile size, rounded up
		int3 tileCount; // number of tiles in current viewport
		int3 topTile; // top-left tile of the viewport
		int3 initPos; // starting drawing position [in pixels]
		int3 pos; // current position [in tiles]
		int3 realPos; // current position [in pixels]
		Rect realTileRect; // default rect based on current pos: [realPos.x, realPos.y, tileSize, tileSize]
		Rect defaultTileRect; // default rect based on 0: [0, 0, tileSize, tileSize]
		const MapDrawingInfo * info; // data for drawing passed from outside

		/// general drawing method, called internally by more specialized ones
		virtual void drawElement(EMapCacheType cacheType, SDL_Surface * sourceSurf, SDL_Rect * sourceRect,
								 SDL_Surface * targetSurf, SDL_Rect * destRect, bool alphaBlit = false, ui8 rotationInfo = 0u) const = 0;

		// first drawing pass

		/// draws terrain bitmap (or custom bitmap if applicable) on current tile
		virtual void drawTileTerrain(SDL_Surface * targetSurf, const TerrainTile & tinfo, const TerrainTile2 & tile) const;
		/// draws a river segment on current tile
		virtual void drawRiver(SDL_Surface * targetSurf, const TerrainTile & tinfo) const;
		/// draws a road segment on current tile
		virtual void drawRoad(SDL_Surface * targetSurf, const TerrainTile & tinfo, const TerrainTile * tinfoUpper) const;
		/// draws all objects on current tile (higher-level logic, unlike other draw*** methods)
		virtual void drawObjects(SDL_Surface * targetSurf, const TerrainTile2 & tile) const;
		virtual void drawObject(SDL_Surface * targetSurf, SDL_Surface * sourceSurf, SDL_Rect * sourceRect, bool moving) const;
		virtual void drawHeroFlag(SDL_Surface * targetSurf, SDL_Surface * sourceSurf, SDL_Rect * sourceRect, SDL_Rect * destRect, bool moving) const;

		// second drawing pass

		/// current tile: draws overlay over the map, used to draw world view icons
		virtual void drawTileOverlay(SDL_Surface * targetSurf, const TerrainTile2 & tile) const = 0;
		/// draws fog of war on current tile
		virtual void drawFow(SDL_Surface * targetSurf) const;
		/// draws map border frame on current position
		virtual void drawFrame(SDL_Surface * targetSurf) const;
		/// draws additional icons (for VIEW_AIR, VIEW_EARTH spells atm)
		virtual void drawOverlayEx(SDL_Surface * targetSurf);

		// third drawing pass

		/// custom post-processing, if needed (used by puzzle view)
		virtual void postProcessing(SDL_Surface * targetSurf) const {}

		// misc methods

		/// initializes frame-drawing (called at the start of every redraw)
		virtual void init(const MapDrawingInfo * drawingInfo) = 0;
		/// calculates clip region for map viewport
		virtual SDL_Rect clip(SDL_Surface * targetSurf) const = 0;

		virtual ui8 getHeroFrameNum(ui8 dir, bool isMoving) const;
		///returns appropriate bitmap and info if alpha blitting is necessary
		virtual std::pair<SDL_Surface *, bool> getVisBitmap() const;
		virtual ui8 getPhaseShift(const CGObjectInstance *object) const;

		virtual bool canDrawObject(const CGObjectInstance * obj) const;
		virtual bool canDrawCurrentTile() const;
		
		// internal helper methods to choose correct bitmap(s) for object; called internally by findObjectBitmap
		AnimBitmapHolder findHeroBitmap(const CGHeroInstance * hero, int anim) const;
		AnimBitmapHolder findBoatBitmap(const CGBoat * hero, int anim) const;		
		SDL_Surface * findFlagBitmap(const CGHeroInstance * obj, int anim, const PlayerColor * color, int indexOffset) const;
		SDL_Surface * findHeroFlagBitmap(const CGHeroInstance * obj, int anim, const PlayerColor * color, int indexOffset) const;
		SDL_Surface * findBoatFlagBitmap(const CGBoat * obj, int anim, const PlayerColor * color, int indexOffset, ui8 dir) const;
		SDL_Surface * findFlagBitmapInternal(const CDefEssential * def, int anim, int indexOffset, ui8 dir, bool moving) const;
		int findAnimIndexByGroup(const CDefEssential * def, int groupNum) const;
		
	public:
		CMapBlitter(CMapHandler * p) : parent(p) {}
		virtual ~CMapBlitter(){}
		void blit(SDL_Surface * targetSurf, const MapDrawingInfo * info);
		/// helper method that chooses correct bitmap(s) for given object
		AnimBitmapHolder findObjectBitmap(const CGObjectInstance * obj, int anim) const;

	};

	class CMapNormalBlitter : public CMapBlitter
	{
	protected:
		void drawElement(EMapCacheType cacheType, SDL_Surface * sourceSurf, SDL_Rect * sourceRect,
						 SDL_Surface * targetSurf, SDL_Rect * destRect, bool alphaBlit = false, ui8 rotationInfo = 0u) const override;

		void drawTileOverlay(SDL_Surface * targetSurf,const TerrainTile2 & tile) const override {}
		void init(const MapDrawingInfo * info) override;
		SDL_Rect clip(SDL_Surface * targetSurf) const override;
	public:
		CMapNormalBlitter(CMapHandler * parent);
		virtual ~CMapNormalBlitter(){}
	};

	class CMapWorldViewBlitter : public CMapBlitter
	{
	private:
		SDL_Surface * objectToIcon(Obj id, si32 subId, PlayerColor owner) const;
	protected:
		void drawElement(EMapCacheType cacheType, SDL_Surface * sourceSurf, SDL_Rect * sourceRect,
						 SDL_Surface * targetSurf, SDL_Rect * destRect, bool alphaBlit = false, ui8 rotationInfo = 0u) const override;

		void drawTileOverlay(SDL_Surface * targetSurf, const TerrainTile2 & tile) const override;
		void drawHeroFlag(SDL_Surface * targetSurf, SDL_Surface * sourceSurf, SDL_Rect * sourceRect, SDL_Rect * destRect, bool moving) const override;
		void drawObject(SDL_Surface * targetSurf, SDL_Surface * sourceSurf, SDL_Rect * sourceRect, bool moving) const override;
		void drawFrame(SDL_Surface * targetSurf) const override {}
		void drawOverlayEx(SDL_Surface * targetSurf) override;
		void init(const MapDrawingInfo * info) override;
		SDL_Rect clip(SDL_Surface * targetSurf) const override;

//		ui8 getHeroFrameNum(ui8 dir, bool isMoving) const override { return 0u; }
		ui8 getPhaseShift(const CGObjectInstance *object) const override { return 0u; }

		void drawScaledRotatedElement(EMapCacheType type, SDL_Surface * baseSurf, SDL_Surface * targetSurf, ui8 rotation,
									  float scale, SDL_Rect * dstRect, SDL_Rect * srcRect = nullptr) const;
		void calculateWorldViewCameraPos();
	public:
		CMapWorldViewBlitter(CMapHandler * parent);
		virtual ~CMapWorldViewBlitter(){}
	};

	class CMapPuzzleViewBlitter : public CMapNormalBlitter
	{
		std::vector<int> unblittableObjects;

		void drawObjects(SDL_Surface * targetSurf, const TerrainTile2 & tile) const override;
		void drawFow(SDL_Surface * targetSurf) const override {} // skipping FoW in puzzle view
		void postProcessing(SDL_Surface * targetSurf) const override;
		bool canDrawObject(const CGObjectInstance * obj) const override;
		bool canDrawCurrentTile() const override { return true; }
	public:
		CMapPuzzleViewBlitter(CMapHandler * parent);
	};

	CMapCache cache;
	CMapBlitter * normalBlitter;
	CMapBlitter * worldViewBlitter;
	CMapBlitter * puzzleViewBlitter;
	
	std::map<int, std::pair<int3, CFadeAnimation*>> fadeAnims;
	int fadeAnimCounter;

	CMapBlitter * resolveBlitter(const MapDrawingInfo * info) const;
	bool updateObjectsFade();
	bool startObjectFade(TerrainTileObject & obj, bool in, int3 pos);
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

	void getTerrainDescr(const int3 &pos, std::string & out, bool terName); //if tername == false => empty string when tile is clear
	CGObjectInstance * createObject(int id, int subid, int3 pos, int owner=254); //creates a new object with a certain id and subid
	bool printObject(const CGObjectInstance * obj, bool fadein = false); //puts appropriate things to tiles, so obj will be visible on map
	bool hideObject(const CGObjectInstance * obj, bool fadeout = false); //removes appropriate things from ttiles, so obj will be no longer visible on map (but still will exist)
	bool removeObject(CGObjectInstance * obj, bool fadeout = false); //removes object from each place in VCMI (I hope)
	void init();
	void calculateBlockedPos();
	void initObjectRects();
	void borderAndTerrainBitmapInit();
	void roadsRiverTerrainInit();
	void prepareFOWDefs();

	EMapAnimRedrawStatus drawTerrainRectNew(SDL_Surface * targetSurface, const MapDrawingInfo * info, bool redrawOnlyAnim = false);
	void updateWater();
	void validateRectTerr(SDL_Rect * val, const SDL_Rect * ext); //terrainRect helper
	static ui8 getDir(const int3 & a, const int3 & b);  //returns direction number in range 0 - 7 (0 is left top, clockwise) [direction: form a to b]
	/// determines if the map is ready to handle new hero movement (not available during fading animations)
	bool canStartHeroMovement();

	void discardWorldViewCache();

	static bool compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b);
};
