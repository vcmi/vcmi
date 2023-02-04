/*
 * maphandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
//code is copied from vcmiclient/mapHandler.h with minimal changes

#include "StdInc.h"
#include "../lib/mapping/CMap.h"
#include "Animation.h"

#include <QImage>
#include <QPixmap>
#include <QRect>

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class CGBoat;
class PlayerColor;

VCMI_LIB_NAMESPACE_END

struct TileObject
{
	CGObjectInstance *obj;
	QRect rect;
	
	TileObject(CGObjectInstance *obj_, QRect rect_);
	~TileObject();
};

using TileObjects = std::vector<TileObject>; //pointers to objects being on this tile with rects to be easier to blit this tile on screen

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
	
	int index(int x, int y, int z) const;
	int index(const int3 &) const;
		
	std::shared_ptr<QImage> findFlagBitmapInternal(std::shared_ptr<Animation> animation, int anim, int group, ui8 dir, bool moving) const;
	std::shared_ptr<QImage> findFlagBitmap(const CGHeroInstance * obj, int anim, const PlayerColor color, int group) const;
	AnimBitmapHolder findObjectBitmap(const CGObjectInstance * obj, int anim, int group = 0) const;
	
	//FIXME: unique_ptr should be enough, but fails to compile in MSVS 2013
	typedef std::map<std::string, std::shared_ptr<Animation>> TFlippedAnimations; //[type, rotation]
	typedef std::map<std::string, std::vector<std::shared_ptr<QImage>>> TFlippedCache;//[type, view type, rotation]
	
	TFlippedAnimations terrainAnimations;//[terrain type, rotation]
	TFlippedCache terrainImages;//[terrain type, view type, rotation]
	
	TFlippedAnimations roadAnimations;//[road type, rotation]
	TFlippedCache roadImages;//[road type, view type, rotation]
	
	TFlippedAnimations riverAnimations;//[river type, rotation]
	TFlippedCache riverImages;//[river type, view type, rotation]
	
	std::vector<TileObjects> ttiles; //informations about map tiles
	int3 sizes; //map size (x = width, y = height, z = number of levels)
	const CMap * map{};
		
	enum class EMapCacheType : char
	{
		TERRAIN, OBJECTS, ROADS, RIVERS, FOW, HEROES, HERO_FLAGS, FRAME, AFTER_LAST
	};
	
	void initObjectRects();
	void initTerrainGraphics();
	QRgb getTileColor(int x, int y, int z);
		
public:
	MapHandler();
	~MapHandler() = default;
	
	void reset(const CMap * Map);

	void updateWater();
	
	void drawTerrainTile(QPainter & painter, int x, int y, int z);
	/// draws a river segment on current tile
	void drawRiver(QPainter & painter, int x, int y, int z);
	/// draws a road segment on current tile
	void drawRoad(QPainter & painter, int x, int y, int z);
	
	void invalidate(int x, int y, int z); //invalidates all objects in particular tile
	void invalidate(CGObjectInstance *); //invalidates object rects
	void invalidate(const std::vector<int3> &); //invalidates all tiles
	void invalidateObjects(); //invalidates all objects on the map
	std::vector<int3> getTilesUnderObject(CGObjectInstance *) const;
	
	/// draws all objects on current tile (higher-level logic, unlike other draw*** methods)
	void drawObjects(QPainter & painter, int x, int y, int z);
	void drawObject(QPainter & painter, const TileObject & object);
	void drawObjectAt(QPainter & painter, const CGObjectInstance * object, int x, int y);
	std::vector<TileObject> & getObjects(int x, int y, int z);
	
	void drawMinimapTile(QPainter & painter, int x, int y, int z);

	static bool compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b);
};
