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
class ObjectInstanceID;
class CGObjectInstance;
struct TerrainTile;

VCMI_LIB_NAMESPACE_END

class IMapRendererContext
{
public:
	virtual ~IMapRendererContext() = default;

	using VisibilityMap = std::shared_ptr<const boost::multi_array<ui8, 3>>;
	using ObjectsVector = std::vector< ConstTransitivePtr<CGObjectInstance> >;

	virtual int3 getMapSize() const = 0;
	virtual bool isInMap(const int3 & coordinates) const = 0;
	virtual const TerrainTile & getMapTile(const int3 & coordinates) const = 0;

	virtual ObjectsVector getAllObjects() const = 0;
	virtual const CGObjectInstance * getObject( ObjectInstanceID objectID ) const = 0;

	virtual bool isVisible(const int3 & coordinates) const = 0;
	virtual VisibilityMap getVisibilityMap() const = 0;

	virtual uint32_t getAnimationPeriod() const = 0;
	virtual uint32_t getAnimationTime() const = 0;
	virtual Point tileSize() const = 0;

	virtual bool showGrid() const = 0;
};
