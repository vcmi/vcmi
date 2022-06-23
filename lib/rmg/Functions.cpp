//
//  Functions.cpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#include "Functions.h"
#include "CMapGenerator.h"
#include "../CTownHandler.h"
#include "../mapping/CMapEditManager.h"

std::set<int3> collectDistantTiles(const Zone& zone, float distance)
{
	std::set<int3> discardedTiles;
	for(auto & tile : zone.getTileInfo())
	{
		if(tile.dist2d(zone.getPos()) > distance)
		{
			discardedTiles.insert(tile);
		}
	};
	return discardedTiles;
}

void createBorder(CMapGenerator & gen, const Zone & zone)
{
	for(auto & tile : zone.getTileInfo())
	{
		bool edge = false;
		gen.foreach_neighbour(tile, [&zone, &edge, &gen](int3 &pos)
		{
			if (edge)
				return; //optimization - do it only once
			if (gen.getZoneID(pos) != zone.getId()) //optimization - better than set search
			{
				//bugfix with missing pos
				if (gen.isPossible(pos))
					gen.setOccupied(pos, ETileType::BLOCKED);
				//we are edge if at least one tile does not belong to zone
				//mark all nearby tiles blocked and we're done
				gen.foreach_neighbour(pos, [&gen](int3 &nearbyPos)
				{
					if (gen.isPossible(nearbyPos))
						gen.setOccupied(nearbyPos, ETileType::BLOCKED);
				});
				edge = true;
			}
		});
	}
}

si32 getRandomTownType(const Zone & zone, CRandomGenerator & generator, bool matchUndergroundType)
{
	auto townTypesAllowed = (zone.getTownTypes().size() ? zone.getTownTypes() : zone.getDefaultTownTypes());
	if(matchUndergroundType)
	{
		std::set<TFaction> townTypesVerify;
		for(TFaction factionIdx : townTypesAllowed)
		{
			bool preferUnderground = (*VLC->townh)[factionIdx]->preferUndergroundPlacement;
			if(zone.isUnderground() ? preferUnderground : !preferUnderground)
			{
				townTypesVerify.insert(factionIdx);
			}
		}
		if(!townTypesVerify.empty())
			townTypesAllowed = townTypesVerify;
	}
	
	return *RandomGeneratorUtil::nextItem(townTypesAllowed, generator);
}

void paintZoneTerrain(const Zone & zone, CMapGenerator & gen, const Terrain & terrainType)
{
	std::vector<int3> tiles(zone.getTileInfo().begin(), zone.getTileInfo().end());
	gen.getEditManager()->getTerrainSelection().setSelection(tiles);
	gen.getEditManager()->drawTerrain(terrainType, &gen.rand);
}

boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>> createPriorityQueue()
{
	return boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>>();
}
