/*
 * WaterAdopter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "WaterAdopter.h"
#include "../CMapGenerator.h"
#include "../RmgMap.h"
#include "../../mapping/CMap.h"
#include "../../mapping/CMapEditManager.h"
#include "../../mapObjects/CObjectClassesHandler.h"
#include "../RmgPath.h"
#include "../RmgObject.h"
#include "ObjectManager.h"
#include "../Functions.h"
#include "RoadPlacer.h"
#include "TreasurePlacer.h"
#include "TownPlacer.h"
#include "ConnectionsPlacer.h"
#include "../TileInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

void WaterAdopter::process()
{
	createWater(map.getMapGenOptions().getWaterContent());
}

void WaterAdopter::init()
{
	//make dependencies
	DEPENDENCY(TownPlacer);
	POSTFUNCTION(ConnectionsPlacer);
	POSTFUNCTION(TreasurePlacer);
}

void WaterAdopter::createWater(EWaterContent::EWaterContent waterContent)
{
	if(waterContent == EWaterContent::NONE || zone.isUnderground() || zone.getType() == ETemplateZoneType::WATER)
		return; //do nothing
	
	distanceMap = zone.area().computeDistanceMap(reverseDistanceMap);
	
	//add border tiles as water for ISLANDS
	if(waterContent == EWaterContent::ISLANDS)
	{
		waterArea.unite(collectDistantTiles(zone, zone.getSize() + 1));
		waterArea.unite(zone.area().getBorder());
	}
	
	//protect some parts from water for NORMAL
	if(waterContent == EWaterContent::NORMAL)
	{
		waterArea.unite(collectDistantTiles(zone, zone.getSize() - 1));
		auto sliceStart = RandomGeneratorUtil::nextItem(reverseDistanceMap[0], zone.getRand());
		auto sliceEnd = RandomGeneratorUtil::nextItem(reverseDistanceMap[0], zone.getRand());
		
		//at least 25% without water
		bool endPassed = false;
		for(int counter = 0; counter < reverseDistanceMap[0].size() / 4 || !endPassed; ++sliceStart, ++counter)
		{
			if(sliceStart == reverseDistanceMap[0].end())
				sliceStart = reverseDistanceMap[0].begin();
			
			if(sliceStart == sliceEnd)
				endPassed = true;
			
			noWaterArea.add(*sliceStart);
		}
		
		rmg::Area noWaterSlice;
		for(int i = 1; i < reverseDistanceMap.size(); ++i)
		{
			for(const auto & t : reverseDistanceMap[i])
			{
				if(noWaterArea.distanceSqr(t) < 3)
					noWaterSlice.add(t);
			}
			noWaterArea.unite(noWaterSlice);
		}
	}
	
	//generating some irregularity of coast
	int coastIdMax = sqrt(reverseDistanceMap.size()); //size of coastTilesMap shows the most distant tile from water
	assert(coastIdMax > 0);
	std::list<int3> tilesQueue;
	rmg::Tileset tilesChecked;
	for(int coastId = coastIdMax; coastId >= 0; --coastId)
	{
		//amount of iterations shall be proportion of coast perimeter
		const int coastLength = reverseDistanceMap[coastId].size() / (coastId + 3);
		for(int coastIter = 0; coastIter < coastLength; ++coastIter)
		{
			int3 tile = *RandomGeneratorUtil::nextItem(reverseDistanceMap[coastId], zone.getRand());
			if(tilesChecked.find(tile) != tilesChecked.end())
				continue;
			
			if(map.isUsed(tile) || map.isFree(tile)) //prevent placing water nearby town
				continue;
			
			tilesQueue.push_back(tile);
			tilesChecked.insert(tile);
		}
	}
	
	//if tile is marked as water - connect it with "big" water
	while(!tilesQueue.empty())
	{
		int3 src = tilesQueue.front();
		tilesQueue.pop_front();
		
		if(waterArea.contains(src))
			continue;
		
		waterArea.add(src);
				
		map.foreach_neighbour(src, [&src, this, &tilesChecked, &tilesQueue](const int3 & dst)
		{
			if(tilesChecked.count(dst))
				return;
			
			if(distanceMap[dst] >= 0 && distanceMap[src] - distanceMap[dst] == 1)
			{
				tilesQueue.push_back(dst);
				tilesChecked.insert(dst);
			}
		});
	}
	
	waterArea.subtract(noWaterArea);
	
	//start filtering of narrow places and coast atrifacts
	rmg::Area waterAdd;
	for(int coastId = 1; coastId <= coastIdMax; ++coastId)
	{
		for(const auto & tile : reverseDistanceMap[coastId])
		{
			//collect neighbout water tiles
			auto collectionLambda = [this](const int3 & t, std::set<int3> & outCollection)
			{
				if(waterArea.contains(t))
				{
					reverseDistanceMap[0].insert(t);
					outCollection.insert(t);
				}
			};
			std::set<int3> waterCoastDirect;
			std::set<int3> waterCoastDiag;
			map.foreachDirectNeighbour(tile, std::bind(collectionLambda, std::placeholders::_1, std::ref(waterCoastDirect)));
			map.foreachDiagonalNeighbour(tile, std::bind(collectionLambda, std::placeholders::_1, std::ref(waterCoastDiag)));
			int waterCoastDirectNum = waterCoastDirect.size();
			int waterCoastDiagNum = waterCoastDiag.size();
			
			//remove tiles which are mostly covered by water
			if(waterCoastDirectNum >= 3)
			{
				waterAdd.add(tile);
				continue;
			}
			if(waterCoastDiagNum == 4 && waterCoastDirectNum == 2)
			{
				waterAdd.add(tile);
				continue;
			}
			if(waterCoastDirectNum == 2 && waterCoastDiagNum >= 2)
			{
				int3 diagSum;
				int3 dirSum;
				for(const auto & i : waterCoastDiag)
					diagSum += i - tile;
				for(const auto & i : waterCoastDirect)
					dirSum += i - tile;
				if(diagSum == int3() || dirSum == int3())
				{
					waterAdd.add(tile);
					continue;
				}
				if(waterCoastDiagNum == 3 && diagSum != dirSum)
				{
					waterAdd.add(tile);
					continue;
				}
			}
		}
	}
	
	waterArea.unite(waterAdd);
	
	//filtering tiny "lakes"
	for(const auto & tile : reverseDistanceMap[0]) //now it's only coast-water tiles
	{
		if(!waterArea.contains(tile)) //for ground tiles
			continue;
		
		std::vector<int3> groundCoast;
		map.foreachDirectNeighbour(tile, [this, &groundCoast](const int3 & t)
		{
			if(!waterArea.contains(t) && zone.area().contains(t)) //for ground tiles of same zone
			{
				groundCoast.push_back(t);
			}
		});
		
		if(groundCoast.size() >= 3)
		{
			waterArea.erase(tile);
		}
		else
		{
			if(groundCoast.size() == 2)
			{
				if(groundCoast[0] + groundCoast[1] == int3())
				{
					waterArea.erase(tile);
				}
			}
		}
	}
	
	{
		Zone::Lock waterLock(map.getZones()[waterZoneId]->areaMutex);
		map.getZones()[waterZoneId]->area().unite(waterArea);
	}
	Zone::Lock lock(zone.areaMutex);
	zone.area().subtract(waterArea);
	zone.areaPossible().subtract(waterArea);
	distanceMap = zone.area().computeDistanceMap(reverseDistanceMap);
}

void WaterAdopter::setWaterZone(TRmgTemplateZoneId water)
{
	waterZoneId = water;
}

rmg::Area WaterAdopter::getCoastTiles() const
{
	if(reverseDistanceMap.empty())
		return rmg::Area();
	
	return rmg::Area(reverseDistanceMap.at(0));
}

char WaterAdopter::dump(const int3 & t)
{
	if(noWaterArea.contains(t))
		return 'X';
	if(waterArea.contains(t))
		return '~';
	
	auto distanceMapIter = distanceMap.find(t);
	if(distanceMapIter != distanceMap.end())
	{
		if(distanceMapIter->second > 9)
			return '%';
		
		auto distStr = std::to_string(distanceMapIter->second);
		if(distStr.length() > 0)
			return distStr[0];
	}
	
	return Modificator::dump(t);
}

VCMI_LIB_NAMESPACE_END
