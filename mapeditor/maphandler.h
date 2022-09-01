#ifndef MAPHANDLER_H
#define MAPHANDLER_H

#include "StdInc.h"
#include "../lib/mapping/CMap.h"
#include "Animation.h"

#include <QImage>
#include <QPixmap>
#include <QRect>

class CGObjectInstance;
class CGBoat;
class PlayerColor;

struct TerrainTileObject
{
	CGObjectInstance *obj;
	QRect rect;
	bool real;
	
	TerrainTileObject(CGObjectInstance *obj_, QRect rect_, bool visitablePos = false);
	~TerrainTileObject();
};

struct TerrainTile2
{
	std::vector<TerrainTileObject> objects; //pointers to objects being on this tile with rects to be easier to blit this tile on screen
};

class MapHandler
{
public:
	struct AnimBitmapHolder
	{
		std::shared_ptr<QImage> objBitmap; // main object bitmap
		std::shared_ptr<QImage> flagBitmap; // flag bitmap for the object (probably only for heroes and boats with heroes)
		
		AnimBitmapHolder(std::shared_ptr<QImage> objBitmap_ = nullptr, std::shared_ptr<QImage> flagBitmap_ = nullptr)
		: objBitmap(objBitmap_),
		flagBitmap(flagBitmap_)
		{}
	};
	
private:
	
	ui8 getHeroFrameGroup(ui8 dir, bool isMoving) const;
	ui8 getPhaseShift(const CGObjectInstance *object) const;
	
	// internal helper methods to choose correct bitmap(s) for object; called internally by findObjectBitmap
	AnimBitmapHolder findHeroBitmap(const CGHeroInstance * hero, int anim) const;
	AnimBitmapHolder findBoatBitmap(const CGBoat * hero, int anim) const;
	std::shared_ptr<QImage> findFlagBitmap(const CGHeroInstance * obj, int anim, const PlayerColor * color, int group) const;
	std::shared_ptr<QImage> findHeroFlagBitmap(const CGHeroInstance * obj, int anim, const PlayerColor * color, int group) const;
	std::shared_ptr<QImage> findBoatFlagBitmap(const CGBoat * obj, int anim, const PlayerColor * color, int group, ui8 dir) const;
	std::shared_ptr<QImage> findFlagBitmapInternal(std::shared_ptr<Animation> animation, int anim, int group, ui8 dir, bool moving) const;
	
public:
	
	AnimBitmapHolder findObjectBitmap(const CGObjectInstance * obj, int anim) const;
	
	enum class EMapCacheType : char
	{
		TERRAIN, OBJECTS, ROADS, RIVERS, FOW, HEROES, HERO_FLAGS, FRAME, AFTER_LAST
	};
	
	void initObjectRects();
	void initTerrainGraphics();

	std::vector<TerrainTile2> ttiles; //informations about map tiles
	int3 sizes; //map size (x = width, y = height, z = number of levels)
	const CMap * map;

	//terrain graphics

	//FIXME: unique_ptr should be enough, but fails to compile in MSVS 2013
	typedef std::map<std::string, std::shared_ptr<Animation>> TFlippedAnimations; //[type, rotation]
	typedef std::map<std::string, std::vector<std::shared_ptr<QImage>>> TFlippedCache;//[type, view type, rotation]

	TFlippedAnimations terrainAnimations;//[terrain type, rotation]
	TFlippedCache terrainImages;//[terrain type, view type, rotation]

	TFlippedAnimations roadAnimations;//[road type, rotation]
	TFlippedCache roadImages;//[road type, view type, rotation]

	TFlippedAnimations riverAnimations;//[river type, rotation]
	TFlippedCache riverImages;//[river type, view type, rotation]
	
	void drawTerrainTile(QPainter & painter, int x, int y, int z);
	/// draws a river segment on current tile
	//void drawRiver(const TerrainTile & tinfo) const;
	/// draws a road segment on current tile
	//void drawRoad(const TerrainTile & tinfo, const TerrainTile * tinfoUpper) const;
	/// draws all objects on current tile (higher-level logic, unlike other draw*** methods)
	void drawObjects(QPainter & painter, int x, int y, int z);
	void drawObject(QPainter & painter, const TerrainTileObject & object);
	void drawObjectAt(QPainter & painter, const CGObjectInstance * object, int x, int y);
	std::vector<TerrainTileObject> & getObjects(int x, int y, int z);
	//void drawObject(SDL_Surface * targetSurf, std::shared_ptr<IImage> source, SDL_Rect * sourceRect, bool moving) const;
	//void drawHeroFlag(SDL_Surface * targetSurf, std::shared_ptr<IImage> source, SDL_Rect * sourceRect, SDL_Rect * destRect, bool moving) const;

	mutable std::map<const CGObjectInstance*, ui8> animationPhase;

	MapHandler(const CMap * Map);
	~MapHandler() = default;
	
	void init();

	//void getTerrainDescr(const int3 & pos, std::string & out, bool isRMB) const; // isRMB = whether Right Mouse Button is clicked
	//bool printObject(const CGObjectInstance * obj, bool fadein = false); //puts appropriate things to tiles, so obj will be visible on map
	//bool hideObject(const CGObjectInstance * obj, bool fadeout = false); //removes appropriate things from ttiles, so obj will be no longer visible on map (but still will exist)
	//bool hasObjectHole(const int3 & pos) const; // Checks if TerrainTile2 tile has a pit remained after digging.

	//EMapAnimRedrawStatus drawTerrainRectNew(SDL_Surface * targetSurface, const MapDrawingInfo * info, bool redrawOnlyAnim = false);
	void updateWater();
	/// determines if the map is ready to handle new hero movement (not available during fading animations)
	//bool canStartHeroMovement();

	//void discardWorldViewCache();

	static bool compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b);
};

#endif // MAPHANDLER_H
