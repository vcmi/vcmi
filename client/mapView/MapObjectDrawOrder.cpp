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

#include "../../lib/int3.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/CMap.h"

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

	return isLowerInAnyTile(map, object, other);
}

void MapObjectDrawOrder::insert(std::vector<ObjectInstanceID> & container, const CMap & map, const CGObjectInstance * object, const int3 & tile)
{
	const ui8 layer = object->drawLayerAt(tile);
	auto position = container.end();

	while(position != container.begin())
	{
		const auto * previous = map.getObject(*(position - 1));

		if(!previous)
			break;

		const ui8 previousLayer = previous->drawLayerAt(tile);

		if(layer > previousLayer)
			break;

		if(layer == previousLayer && !isDrawnBelow(map, object, previous))
			break;

		--position;
	}
	container.insert(position, object->id);
}
