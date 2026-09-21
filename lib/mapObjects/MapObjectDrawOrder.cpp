/*
 * MapObjectDrawOrder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapObjectDrawOrder.h"

#include "ObjectTemplate.h"
#include "../mapping/CMap.h"

/// Whether some tile covered by both objects has a lower layer of the first object than of the second one
static bool isLowerInAnyTile(const CMap & map, const CGObjectInstance * object, const CGObjectInstance * other)
{
	const int3 anchor = object->anchorPos();
	const int3 otherAnchor = other->anchorPos();

	const int left = std::max(anchor.x - object->getWidth() + 1, otherAnchor.x - other->getWidth() + 1);
	const int right = std::min(anchor.x, otherAnchor.x);
	const int top = std::max(anchor.y - object->getHeight() + 1, otherAnchor.y - other->getHeight() + 1);
	const int bottom = std::min(anchor.y, otherAnchor.y);

	for(int x = left; x <= right; ++x)
	{
		for(int y = top; y <= bottom; ++y)
		{
			const int3 tile(x, y, anchor.z);

			if(map.isInTheMap(tile) && object->drawLayerAt(tile) < other->drawLayerAt(tile))
				return true;
		}
	}
	return false;
}

/// Whether the object goes below the one that is already on the tile in the same layer
static bool isDrawnBelow(const CMap & map, const CGObjectInstance * object, const CGObjectInstance * other)
{
	// objects with print priority are drawn in order of it, the higher first
	const si32 priority = object->appearance->printPriority;
	const si32 otherPriority = other->appearance->printPriority;

	if(priority != otherPriority)
		return priority > otherPriority;

	if(isLowerInAnyTile(map, object, other))
		return true;

	// objects that are added again, e.g. after a change of owner, keep the place of their load order
	return !isLowerInAnyTile(map, other, object) && object->id < other->id;
}

bool MapObjectDrawOrder::usesFixedDrawSlot(const CGObjectInstance * object)
{
	return object->ID == Obj::HERO || object->ID == Obj::BOAT;
}

bool MapObjectDrawOrder::isSpecialGround(const CGObjectInstance * object)
{
	// all objects of the "terrain" handler, mods can add their own
	return object->isTile2Terrain();
}

bool MapObjectDrawOrder::goesBelow(const CMap & map, const CGObjectInstance * object, const CGObjectInstance * previous, const int3 & tile)
{
	const ui8 layer = object->drawLayerAt(tile);
	const ui8 previousLayer = previous->drawLayerAt(tile);

	return layer < previousLayer || (layer == previousLayer && isDrawnBelow(map, object, previous));
}

const CGObjectInstance * MapObjectDrawOrder::findTopObject(const CMap & map, std::vector<const CGObjectInstance *> objects, const int3 & tile)
{
	// objects are stamped onto the map in the order of their ids
	std::ranges::sort(objects, {}, &CGObjectInstance::id);

	const CGObjectInstance * top = nullptr;

	for(const auto * object : objects)
	{
		if(!top || (usesFixedDrawSlot(object) && !usesFixedDrawSlot(top)) || (usesFixedDrawSlot(object) == usesFixedDrawSlot(top) && !goesBelow(map, object, top, tile)))
			top = object;
	}
	return top;
}
