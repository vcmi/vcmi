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

#include "../lib/int3.h"
#include "Animation.h"

#include <QImage>
#include <QPixmap>
#include <QRect>

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class CGHeroInstance;
class CGBoat;
class CMap;
class PlayerColor;

VCMI_LIB_NAMESPACE_END

struct ObjectRect
{
	const CGObjectInstance * obj;
	QRect rect;
	
	ObjectRect(const CGObjectInstance * obj_, QRect rect_);
	~ObjectRect();
};

using TileObjects = std::vector<ObjectRect>; //pointers to objects being on this tile with rects to be easier to blit this tile on screen

class MapHandler
{
public:
	struct BitmapHolder
	{
		std::shared_ptr<QImage> objBitmap; // main object bitmap
		std::shared_ptr<QImage> flagBitmap; // flag bitmap for the object (probably only for heroes and boats with heroes)
		
		BitmapHolder(std::shared_ptr<QImage> objBitmap_ = nullptr, std::shared_ptr<QImage> flagBitmap_ = nullptr)
		: objBitmap(objBitmap_),
		flagBitmap(flagBitmap_)
		{}
	};
	
private:
	
	int index(int x, int y, int z) const;
	int index(const int3 &) const;
		
	std::shared_ptr<QImage> findFlagBitmapInternal(std::shared_ptr<Animation> animation, int anim, int group, ui8 dir, bool moving) const;
	std::shared_ptr<QImage> findFlagBitmap(const CGHeroInstance * obj, int anim, const PlayerColor color, int group) const;
	BitmapHolder findObjectBitmap(const CGObjectInstance * obj, int anim, int group = 0) const;
	
	//FIXME: unique_ptr should be enough, but fails to compile in MSVS 2013
	typedef std::map<std::string, std::shared_ptr<Animation>> TFlippedAnimations; //[type, rotation]
	typedef std::map<std::string, std::vector<std::shared_ptr<QImage>>> TFlippedCache;//[type, view type, rotation]
	
	TFlippedAnimations terrainAnimations;//[terrain type, rotation]
	TFlippedCache terrainImages;//[terrain type, view type, rotation]
	
	TFlippedAnimations roadAnimations;//[road type, rotation]
	TFlippedCache roadImages;//[road type, view type, rotation]
	
	TFlippedAnimations riverAnimations;//[river type, rotation]
	TFlippedCache riverImages;//[river type, view type, rotation]
	
	std::vector<TileObjects> tileObjects; //information about map tiles
	std::map<const CGObjectInstance *, std::set<int3>> tilesCache; //set of tiles belonging to object
	
	const CMap * map = nullptr;
	
	void initObjectRects();
	void initTerrainGraphics();
	QRgb getTileColor(int x, int y, int z);
	
	std::shared_ptr<QImage> getObjectImage(const CGObjectInstance * obj);
		
public:
	MapHandler();
	~MapHandler() = default;
	
	void reset(const CMap * Map);
	
	void drawTerrainTile(QPainter & painter, int x, int y, int z, QPointF offset);
	/// draws a river segment on current tile
	void drawRiver(QPainter & painter, int x, int y, int z, QPointF offset);
	/// draws a road segment on current tile
	void drawRoad(QPainter & painter, int x, int y, int z, QPointF offset);
	
	std::set<int3> invalidate(const CGObjectInstance *); //invalidates object rects
	void invalidateObjects(); //invalidates all objects on the map
	const std::set<int3> & getTilesUnderObject(const CGObjectInstance *) const;
	
	//get objects at position
	std::vector<ObjectRect> & getObjects(const int3 & tile);
	std::vector<ObjectRect> & getObjects(int x, int y, int z);
	
	//returns set of tiles to draw
	std::set<int3> removeObject(const CGObjectInstance * object);
	std::set<int3> addObject(const CGObjectInstance * object);
	
	/// draws all objects on current tile (higher-level logic, unlike other draw*** methods)
	void drawObjects(QPainter & painter, const QRectF & section, int z, std::set<const CGObjectInstance *> & locked);
	void drawObjectAt(QPainter & painter, const CGObjectInstance * object, int x, int y, QPointF offset, bool locked = false);
	
	void drawMinimapTile(QPainter & painter, int x, int y, int z);
};
