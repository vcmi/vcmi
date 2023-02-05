/*
 * mapHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once


#include "../../lib/int3.h"
#include "../../lib/spells/ViewSpellInt.h"
#include "../../lib/Rect.h"

#ifdef IN
#undef IN
#endif

#ifdef OUT
#undef OUT
#endif

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class CGHeroInstance;
class CGBoat;
class CMap;
struct TerrainTile;
class PlayerColor;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
class CAnimation;
class IImage;
class CFadeAnimation;

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
	Rect rect;
	int fadeAnimKey;
	boost::optional<std::string> ambientSound;

	TerrainTileObject(const CGObjectInstance *obj_, Rect rect_, bool visitablePos = false);
	~TerrainTileObject();
};

struct TerrainTile2
{
	std::vector<TerrainTileObject> objects; //pointers to objects being on this tile with rects to be easier to blit this tile on screen
};

struct MapDrawingInfo
{
	bool scaled;
	int3 &topTile; // top-left tile in viewport [in tiles]
	std::shared_ptr<const boost::multi_array<ui8, 3>> visibilityMap;
	Rect drawBounds; // map rect drawing bounds on screen
	std::shared_ptr<CAnimation> icons; // holds overlay icons for world view mode
	float scale; // map scale for world view mode (only if scaled == true)

	bool otherheroAnim;
	ui8 anim;
	ui8 heroAnim;

	int3 movement; // used for smooth map movement

	bool puzzleMode;
	int3 grailPos; // location of grail for puzzle mode [in tiles]

	const std::vector<ObjectPosInfo> * additionalIcons;

	bool showAllTerrain; //for expert viewEarth

	MapDrawingInfo(int3 &topTile_, std::shared_ptr<const boost::multi_array<ui8, 3>> visibilityMap_, const Rect & drawBounds_, std::shared_ptr<CAnimation> icons_ = nullptr)
		: scaled(false),
		  topTile(topTile_),

		  visibilityMap(visibilityMap_),
		  drawBounds(drawBounds_),
		  icons(icons_),
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
		return static_cast<int>(inver.size());
	}

private:
	int offset;
	std::vector<T> inver;
};
class CMapHandler
{
	enum class EMapCacheType : ui8
	{
		TERRAIN, OBJECTS, ROADS, RIVERS, FOW, HEROES, HERO_FLAGS, FRAME, AFTER_LAST
	};

	/// temporarily caches rescaled frames for map world view redrawing
	class CMapCache
	{
		std::array< std::map<intptr_t, std::shared_ptr<IImage>>, (ui8)EMapCacheType::AFTER_LAST> data;
		float worldViewCachedScale{0};
	public:
		CMapCache();
		/// destroys all cached data (frees surfaces)
		void discardWorldViewCache();
		/// updates scale and determines if currently cached data is still valid
		void updateWorldViewScale(float scale);
		/// asks for cached data; @returns cached data if found, new scaled surface otherwise, may return nullptr in case of scaling error
		std::shared_ptr<IImage> requestWorldViewCacheOrCreate(EMapCacheType type, std::shared_ptr<IImage> fullSurface);
	};

	/// helper struct to pass around resolved bitmaps of an object; images can be nullptr if object doesn't have bitmap of that type
	struct AnimBitmapHolder
	{
		std::shared_ptr<IImage> objBitmap; // main object bitmap
		std::shared_ptr<IImage> flagBitmap; // flag bitmap for the object (probably only for heroes and boats with heroes)
		bool isMoving; // indicates if the object is moving (again, heroes/boats only)

		AnimBitmapHolder(std::shared_ptr<IImage> objBitmap_ = nullptr, std::shared_ptr<IImage> flagBitmap_ = nullptr, bool moving = false)
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
		virtual void drawElement(EMapCacheType cacheType, std::shared_ptr<IImage> source, Rect * sourceRect, SDL_Surface * targetSurf, Rect * destRect) const = 0;

		// first drawing pass

		/// draws terrain bitmap (or custom bitmap if applicable) on current tile
		virtual void drawTileTerrain(SDL_Surface * targetSurf, const TerrainTile & tinfo, const TerrainTile2 & tile) const;
		/// draws a river segment on current tile
		virtual void drawRiver(SDL_Surface * targetSurf, const TerrainTile & tinfo) const;
		/// draws a road segment on current tile
		virtual void drawRoad(SDL_Surface * targetSurf, const TerrainTile & tinfo, const TerrainTile * tinfoUpper) const;
		/// draws all objects on current tile (higher-level logic, unlike other draw*** methods)
		virtual void drawObjects(SDL_Surface * targetSurf, const TerrainTile2 & tile) const;
		virtual void drawObject(SDL_Surface * targetSurf, std::shared_ptr<IImage> source, Rect * sourceRect, bool moving) const;
		virtual void drawHeroFlag(SDL_Surface * targetSurf, std::shared_ptr<IImage> source, Rect * sourceRect, Rect * destRect, bool moving) const;

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
		virtual Rect clip(SDL_Surface * targetSurf) const = 0;

		virtual ui8 getHeroFrameGroup(ui8 dir, bool isMoving) const;
		virtual ui8 getPhaseShift(const CGObjectInstance *object) const;

		virtual bool canDrawObject(const CGObjectInstance * obj) const;
		virtual bool canDrawCurrentTile() const;

		// internal helper methods to choose correct bitmap(s) for object; called internally by findObjectBitmap
		AnimBitmapHolder findHeroBitmap(const CGHeroInstance * hero, int anim) const;
		AnimBitmapHolder findBoatBitmap(const CGBoat * hero, int anim) const;
		std::shared_ptr<IImage> findFlagBitmap(const CGHeroInstance * obj, int anim, const PlayerColor * color, int group) const;
		std::shared_ptr<IImage> findHeroFlagBitmap(const CGHeroInstance * obj, int anim, const PlayerColor * color, int group) const;
		std::shared_ptr<IImage> findBoatFlagBitmap(const CGBoat * obj, int anim, const PlayerColor * color, int group, ui8 dir) const;
		std::shared_ptr<IImage> findFlagBitmapInternal(std::shared_ptr<CAnimation> animation, int anim, int group, ui8 dir, bool moving) const;

	public:
		CMapBlitter(CMapHandler * p);
		virtual ~CMapBlitter();
		void blit(SDL_Surface * targetSurf, const MapDrawingInfo * info);
		/// helper method that chooses correct bitmap(s) for given object
		AnimBitmapHolder findObjectBitmap(const CGObjectInstance * obj, int anim) const;
	};

	class CMapNormalBlitter : public CMapBlitter
	{
	protected:
		void drawElement(EMapCacheType cacheType, std::shared_ptr<IImage> source, Rect * sourceRect, SDL_Surface * targetSurf, Rect * destRect) const override;
		void drawTileOverlay(SDL_Surface * targetSurf,const TerrainTile2 & tile) const override {}
		void init(const MapDrawingInfo * info) override;
		Rect clip(SDL_Surface * targetSurf) const override;
	public:
		CMapNormalBlitter(CMapHandler * parent);
		virtual ~CMapNormalBlitter(){}
	};

	class CMapWorldViewBlitter : public CMapBlitter
	{
	private:
		std::shared_ptr<IImage> objectToIcon(Obj id, si32 subId, PlayerColor owner) const;
	protected:
		void drawElement(EMapCacheType cacheType, std::shared_ptr<IImage> source, Rect * sourceRect, SDL_Surface * targetSurf, Rect * destRect) const override;
		void drawTileOverlay(SDL_Surface * targetSurf, const TerrainTile2 & tile) const override;
		void drawHeroFlag(SDL_Surface * targetSurf, std::shared_ptr<IImage> source, Rect * sourceRect, Rect * destRect, bool moving) const override;
		void drawObject(SDL_Surface * targetSurf, std::shared_ptr<IImage> source, Rect * sourceRect, bool moving) const override;
		void drawFrame(SDL_Surface * targetSurf) const override {}
		void drawOverlayEx(SDL_Surface * targetSurf) override;
		void init(const MapDrawingInfo * info) override;
		Rect clip(SDL_Surface * targetSurf) const override;
		ui8 getPhaseShift(const CGObjectInstance *object) const override { return 0u; }
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
	int fadeAnimCounter{0};

	CMapBlitter * resolveBlitter(const MapDrawingInfo * info) const;
	bool updateObjectsFade();
	bool startObjectFade(TerrainTileObject & obj, bool in, int3 pos);

	void initObjectRects();
	void initBorderGraphics();
	void initTerrainGraphics();
	void prepareFOWDefs();
public:
	boost::multi_array<TerrainTile2, 3> ttiles; //informations about map tiles [z][x][y]
	int3 sizes; //map size (x = width, y = height, z = number of levels)
	const CMap * map{nullptr};

	// Max number of tiles that will fit in the map screen. Tiles
	// can be partial on each edges.
	int tilesW;
	int tilesH;

	// size of each side of the frame around the whole map, in tiles
	int frameH{};
	int frameW;

	// Coord in pixels of the top left corner of the top left tile to
	// draw. Values range is [-31..0]. A negative value
	// implies that part of the tile won't be displayed.
	int offsetX;
	int offsetY;

	//terrain graphics

	//FIXME: unique_ptr should be enough, but fails to compile in MSVS 2013
	typedef std::map<std::string, std::array<std::shared_ptr<CAnimation>, 4>> TFlippedAnimations; //[type, rotation]
	typedef std::map<std::string, std::vector<std::array<std::shared_ptr<IImage>, 4>>> TFlippedCache;//[type, view type, rotation]

	TFlippedAnimations terrainAnimations;//[terrain type, rotation]
	TFlippedCache terrainImages;//[terrain type, view type, rotation]

	TFlippedAnimations roadAnimations;//[road type, rotation]
	TFlippedCache roadImages;//[road type, view type, rotation]

	TFlippedAnimations riverAnimations;//[river type, rotation]
	TFlippedCache riverImages;//[river type, view type, rotation]

	//Fog of War cache (not owned)
	std::vector<std::shared_ptr<IImage>> FoWfullHide;
	boost::multi_array<ui8, 3> hideBitmap; //frame indexes (in FoWfullHide) of graphic that should be used to fully hide a tile

	std::vector<std::shared_ptr<IImage>> FoWpartialHide;

	//edge graphics
	std::unique_ptr<CAnimation> egdeAnimation;
	std::vector<std::shared_ptr<IImage>> egdeImages;//cache of links to egdeAnimation (for faster access)
	PseudoV< PseudoV< PseudoV <ui8> > > edgeFrames; //frame indexes (in egdeImages) of tile outside of map

	mutable std::map<const CGObjectInstance*, ui8> animationPhase;

	CMapHandler();
	~CMapHandler();

	void getTerrainDescr(const int3 & pos, std::string & out, bool isRMB) const; // isRMB = whether Right Mouse Button is clicked
	bool printObject(const CGObjectInstance * obj, bool fadein = false); //puts appropriate things to tiles, so obj will be visible on map
	bool hideObject(const CGObjectInstance * obj, bool fadeout = false); //removes appropriate things from ttiles, so obj will be no longer visible on map (but still will exist)
	bool hasObjectHole(const int3 & pos) const; // Checks if TerrainTile2 tile has a pit remained after digging.
	void init();

	EMapAnimRedrawStatus drawTerrainRectNew(SDL_Surface * targetSurface, const MapDrawingInfo * info, bool redrawOnlyAnim = false);
	void updateWater();
	/// determines if the map is ready to handle new hero movement (not available during fading animations)
	bool canStartHeroMovement();

	void discardWorldViewCache();

	static bool compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b);
};
