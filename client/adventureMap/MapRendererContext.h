/*
 * MapRenderer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/ConstTransitivePtr.h"

VCMI_LIB_NAMESPACE_BEGIN

class int3;
class Point;
class CGObjectInstance;
class CGHeroInstance;
class ObjectInstanceID;
struct TerrainTile;
struct CGPath;

VCMI_LIB_NAMESPACE_END

class IMapRendererContext
{
public:
	using MapObject = ObjectInstanceID;
	using MapObjectsList = std::vector<MapObject>;

	virtual ~IMapRendererContext() = default;

	/// returns dimensions of current map
	virtual int3 getMapSize() const = 0;

	/// returns true if chosen coordinates exist on map
	virtual bool isInMap(const int3 & coordinates) const = 0;

	/// returns tile by selected coordinates. Coordinates MUST be valid
	virtual const TerrainTile & getMapTile(const int3 & coordinates) const = 0;

	/// returns all objects visible on specified tile
	virtual const MapObjectsList & getObjects(const int3 & coordinates) const = 0;

	/// returns specific object by ID, or nullptr if not found
	virtual const CGObjectInstance * getObject(ObjectInstanceID objectID) const = 0;

	/// returns path of currently active hero, or nullptr if none
	virtual const CGPath * currentPath() const = 0;

	/// returns true if specified tile is visible in current context
	virtual bool isVisible(const int3 & coordinates) const = 0;

	virtual size_t objectGroupIndex(ObjectInstanceID objectID) const = 0;
	virtual Point objectImageOffset(ObjectInstanceID objectID, const int3 & coordinates) const = 0;

	/// returns object animation transparency. IF set to 0, object will not be visible
	virtual double objectTransparency(ObjectInstanceID objectID) const = 0;

	/// returns animation frame for selected object
	virtual size_t objectImageIndex(ObjectInstanceID objectID, size_t groupSize) const = 0;

	/// returns animation frame for terrain
	virtual size_t terrainImageIndex(size_t groupSize) const = 0;

	/// returns size of ouput tile, in pixels. 32x32 for "standard" map, may be smaller for world view mode
	virtual Point getTileSize() const = 0;

	/// if true, map grid should be visible on map
	virtual bool showGrid() const = 0;
	virtual bool showVisitable() const = 0;
	virtual bool showBlockable() const = 0;
};

class IMapObjectObserver
{
public:
	IMapObjectObserver();
	virtual ~IMapObjectObserver();

	virtual bool hasOngoingAnimations() = 0;

	/// Plays fade-in animation and adds object to map
	virtual void onObjectFadeIn(const CGObjectInstance * obj) {}

	/// Plays fade-out animation and removed object from map
	virtual void onObjectFadeOut(const CGObjectInstance * obj) {}

	/// Adds object to map instantly, with no animation
	virtual void onObjectInstantAdd(const CGObjectInstance * obj) {}

	/// Removes object from map instantly, with no animation
	virtual void onObjectInstantRemove(const CGObjectInstance * obj) {}

	/// Perform hero teleportation animation with terrain fade animation
	virtual void onHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest) {}

	/// Perform hero movement animation, moving hero across terrain
	virtual void onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest) {}

	/// Instantly rotates hero to face destination tile
	virtual void onHeroRotated(const CGHeroInstance * obj, const int3 & from, const int3 & dest) {}
};
