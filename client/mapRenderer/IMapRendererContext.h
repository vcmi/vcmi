/*
 * IMapRendererContext.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class int3;
class Point;
class CGObjectInstance;
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

//	/// returns true if selected tile has animation and should not be cached
//	virtual bool tileAnimated(const int3 & coordinates) const = 0;

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
	virtual double objectTransparency(ObjectInstanceID objectID, const int3 &coordinates) const = 0;

	/// returns animation frame for selected object
	virtual size_t objectImageIndex(ObjectInstanceID objectID, size_t groupSize) const = 0;

	/// returns index of image for overlay on specific tile, or numeric_limits::max if none
	virtual size_t overlayImageIndex(const int3 & coordinates) const = 0;

	/// returns animation frame for terrain
	virtual size_t terrainImageIndex(size_t groupSize) const = 0;

//	/// returns size of ouput tile, in pixels. 32x32 for "standard" map, may be smaller for world view mode
//	virtual Point getTileSize() const = 0;

	/// if true, world view overlay will be shown
	virtual bool showOverlay() const = 0;

	/// if true, map grid should be visible on map
	virtual bool showGrid() const = 0;
	virtual bool showVisitable() const = 0;
	virtual bool showBlockable() const = 0;
};
