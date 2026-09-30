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
	/// Hero or boat image offset (in pixels) from which only its top reaches the tile
	constexpr int fixedSlotBodyRowLimit = 16;

	/// Heroes and boats are not ordered with other objects, they have their own slot on the tile
	DLL_LINKAGE bool usesFixedDrawSlot(const CGObjectInstance * object);

	/// Objects that change the terrain under them (cursed ground, magic plains...), drawn below all other objects
	DLL_LINKAGE bool isSpecialGround(const CGObjectInstance * object);

	/// Whether the object stamped onto a tile goes below the one that is already there
	DLL_LINKAGE bool goesBelow(const CMap & map, const CGObjectInstance * object, const CGObjectInstance * previous, const int3 & tile);

	/// The object that is drawn on top of the others on the tile, null if there are none
	DLL_LINKAGE const CGObjectInstance * findTopObject(const CMap & map, std::vector<const CGObjectInstance *> objects, const int3 & tile);

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

	/// One image to draw on a tile: shadow or body of the object
	struct DrawStep
	{
		const CGObjectInstance * object;
		bool isShadow;
	};

	using DrawSteps = boost::container::small_vector<DrawStep, 16>;

	/// Images of one tile in H3 draw order. The entries must be ordered by findInsertPosition
	template<typename Entries, typename GetObject, typename SlotOffset>
	DrawSteps getDrawSteps(const Entries & entries, const int3 & tile, GetObject getObject, SlotOffset slotOffset)
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

			if(!usesFixedDrawSlot(object))
			{
				ordered.push_back(object);
				continue;
			}

			const Point offset = slotOffset(object);
			(offset.y < fixedSlotBodyRowLimit ? bodyRow : headRow).emplace_back(offset.x, object);
		}

		DrawSteps steps;

		const auto addLayers = [&](ui8 lowest, ui8 highest)
		{
			for(const auto * object : ordered)
			{
				const ui8 layer = object->drawLayerAt(tile);

				if(layer >= lowest && layer <= highest)
					steps.push_back({object, false});
			}
		};

		const auto addSlot = [&](SlotObjects & slot)
		{
			std::ranges::stable_sort(slot, {}, &SlotObjects::value_type::first);

			for(const auto & entry : slot)
				steps.push_back({entry.second, false});
		};

		addLayers(0, 0);

		// Like in H3 all shadows of the tile go after the bottom layer and under all the other bodies, so that
		// a shadow never darkens another object - including the shadows of the bottom layer objects themselves
		for(const auto * object : ordered)
			steps.push_back({object, true});

		if(bodyRow.empty() && headRow.empty())
		{
			addLayers(1, 255);
			return steps;
		}

		addLayers(1, 1);
		addSlot(bodyRow);
		addLayers(2, 2);
		addSlot(headRow);
		addLayers(3, 255);
		return steps;
	}

	/// Calls drawShadow(object) or drawBody(object) for every image of the tile, see getDrawSteps
	template<typename Entries, typename GetObject, typename SlotOffset, typename DrawShadow, typename DrawBody>
	void drawTile(const Entries & entries, const int3 & tile, GetObject getObject, SlotOffset slotOffset, DrawShadow drawShadow, DrawBody drawBody)
	{
		for(const auto & step : getDrawSteps(entries, tile, getObject, slotOffset))
		{
			if(step.isShadow)
				drawShadow(step.object);
			else
				drawBody(step.object);
		}
	}
}
