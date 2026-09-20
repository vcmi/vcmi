/*
 * MapObjectDrawOrder.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CGObjectInstance.h"

#include "../GameConstants.h"
#include "../Point.h"
#include "../int3.h"

#include <boost/container/small_vector.hpp>

class CMap;

/// Order in which adventure map objects are drawn, like in H3. Shared by the client and the map editor
namespace MapObjectDrawOrder
{
	/// Heroes and boats are not ordered with other objects, they have their own slot on the tile
	DLL_LINKAGE bool usesFixedDrawSlot(const CGObjectInstance * object);

	/// Objects that change the terrain under them (cursed ground, magic plains and the like) are ground rather than things standing on it. Like in H3 they are
	/// drawn before the rivers and roads, which show over them, and below all other objects
	DLL_LINKAGE bool isSpecialGround(const CGObjectInstance * object);

	/// Whether the object stamped onto a tile goes below the one that is already there
	DLL_LINKAGE bool goesBelow(const CMap & map, const CGObjectInstance * object, const CGObjectInstance * previous, const int3 & tile);

	/// Position in the list of a tile at which H3 puts the object when it stamps it onto the map
	template<typename Container, typename GetObject>
	auto findInsertPosition(Container & container, const CMap & map, const CGObjectInstance * object, const int3 & tile, GetObject getObject)
	{
		auto position = container.end();

		while(position != container.begin())
		{
			const CGObjectInstance * previous = getObject(*(position - 1));

			if(!previous || !goesBelow(map, object, previous, tile))
				break;

			--position;
		}
		return position;
	}

	/// Calls the functors in the order in which H3 draws one tile. The objects must be ordered by findInsertPosition
	/// getObject(entry) gives the object of an entry, slotOffset(object) the offset of its image on the tile,
	/// drawShadow(object) and drawBody(object) draw the image of the object
	template<typename Entries, typename GetObject, typename SlotOffset, typename DrawShadow, typename DrawBody>
	void drawTile(const Entries & entries, const int3 & tile, GetObject getObject, SlotOffset slotOffset, DrawShadow drawShadow, DrawBody drawBody)
	{
		// Like in H3 heroes and boats are drawn between fixed layers of the tile. Each is kept
		// with its horizontal offset, which orders neighbours from left to right
		using SlotObjects = boost::container::small_vector<std::pair<int, const CGObjectInstance *>, 4>;
		SlotObjects bodyRow; // tile is in the row on which the hero stands
		SlotObjects headRow; // tile is above that row, only the top of the hero reaches it
		boost::container::small_vector<const CGObjectInstance *, 8> ordered;

		for(const auto & entry : entries)
		{
			const CGObjectInstance * object = getObject(entry);

			if(!object || isSpecialGround(object))
				continue;

			if(usesFixedDrawSlot(object))
			{
				const Point offset = slotOffset(object);
				auto & slot = offset.y < 16 ? bodyRow : headRow;
				slot.emplace_back(offset.x, object);
			}
			else
				ordered.push_back(object);
		}

		const auto drawLayers = [&](ui8 lowest, ui8 highest)
		{
			for(const auto * object : ordered)
			{
				const ui8 layer = object->drawLayerAt(tile);

				if(layer >= lowest && layer <= highest)
					drawBody(object);
			}
		};

		const auto drawSlot = [&](SlotObjects & slot)
		{
			std::stable_sort(slot.begin(), slot.end(), [](const auto & left, const auto & right) { return left.first < right.first; });

			for(const auto & entry : slot)
				drawBody(entry.second);
		};

		drawLayers(0, 0);

		// Like in H3 all shadows of the tile go after the bottom layer and under all the other bodies, so that
		// a shadow never darkens another object - including the shadows of the bottom layer objects themselves
		for(const auto * object : ordered)
			drawShadow(object);

		if(bodyRow.empty() && headRow.empty())
		{
			drawLayers(1, 255);
		}
		else
		{
			drawLayers(1, 1);
			drawSlot(bodyRow);
			drawLayers(2, 2);
			drawSlot(headRow);
			drawLayers(3, 255);
		}
	}
}
