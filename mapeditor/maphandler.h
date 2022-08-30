#ifndef MAPHANDLER_H
#define MAPHANDLER_H

#include "StdInc.h"
#include "../lib/mapping/CMap.h"
#include "Animation.h"

#include <QImage>
#include <QPixmap>

class MapHandler
{
public:
	enum class EMapCacheType : char
	{
		TERRAIN, OBJECTS, ROADS, RIVERS, FOW, HEROES, HERO_FLAGS, FRAME, AFTER_LAST
	};
	
	void initObjectRects();
	void initTerrainGraphics();

	int3 sizes; //map size (x = width, y = height, z = number of levels)
	const CMap * map;
	QPixmap surface;
	QPainter painter;

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
	
	void drawTerrainTile(int x, int y, const TerrainTile & tinfo);

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
