/*
 * RiverPlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "RiverPlacer.h"
#include "Functions.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "RmgPath.h"
#include "ObjectManager.h"
#include "ObstaclePlacer.h"
#include "WaterProxy.h"

void RiverPlacer::process()
{
	preprocess();
	for(auto & t : riverNodes)
		connectRiver(t);
	
	if(!rivers.empty())
		drawRivers();
}

void RiverPlacer::init()
{
	for(auto & z : map.getZones())
	{
		dependency(z.second->getModificator<WaterProxy>());
	}
	dependency(zone.getModificator<ObjectManager>());
	dependency(zone.getModificator<ObstaclePlacer>());
}

void RiverPlacer::drawRivers()
{
	map.getEditManager()->getTerrainSelection().setSelection(rivers.getTilesVector());
	map.getEditManager()->drawRiver("rw", &generator.rand);
}

char RiverPlacer::dump(const int3 & t)
{
	if(riverNodes.count(t))
		return '@';
	if(rivers.contains(t))
		return '~';
	if(sink.contains(t))
		return '*';
	if(source.contains(t))
		return '^';
	if(zone.area().contains(t))
		return ' ';
	return '?';
}

void RiverPlacer::addRiverNode(const int3 & node)
{
	assert(zone.area().contains(node));
	riverNodes.insert(node);
}

rmg::Area & RiverPlacer::riverSource()
{
	return source;
}

rmg::Area & RiverPlacer::riverSink()
{
	return sink;
}

void RiverPlacer::prepareHeightmap()
{
	for(auto & t : zone.area().getTilesVector())
	{
		heightMap[t] = generator.rand.nextInt(30);
	}
}
 
void RiverPlacer::prepareBorderHeightmap()
{
	std::map<int3, int> heightMap;
	std::vector<int3> border{zone.getArea().getBorder().begin(), zone.getArea().getBorder().end()};
	
	heightMap[*border.begin()] = generator.rand.nextInt(-100, 100);
	heightMap[*std::prev(border.end())] = heightMap[*border.begin()];
	
	auto midpoint = std::next(border.begin(), border.size() / 2);
	heightMap[*midpoint] = generator.rand.nextInt(-100, 100);
		
	prepareBorderHeightmap(border.begin(), midpoint);
	prepareBorderHeightmap(midpoint, std::prev(border.end()));
	
	auto minel = std::min_element(heightMap.begin(), heightMap.end())->second;
	auto maxel = std::max_element(heightMap.begin(), heightMap.end())->second;
	
	if(minel > 0)
	{
		for(auto & i : heightMap)
			i.second -= minel;
	}
	
	if(maxel <= 0)
	{
		for(auto & i : heightMap)
			i.second += maxel + 1;
	}
}

void RiverPlacer::prepareBorderHeightmap(std::vector<int3>::iterator l, std::vector<int3>::iterator r)
{
	if((r - l) < 2)
		return;
	
	auto midpoint = std::next(l, (r - l) / 2);
	heightMap[*midpoint] = (heightMap[*l] + heightMap[*r]) / 2;
	heightMap[*midpoint] += generator.rand.nextInt(-100, 100);
	
	prepareBorderHeightmap(l, midpoint);
	prepareBorderHeightmap(midpoint, r);
}

void RiverPlacer::preprocess()
{
	//decorative river
	if(!sink.empty() && !source.empty() && riverNodes.empty())
	{
		addRiverNode(*RandomGeneratorUtil::nextItem(zone.areaPossible().getTilesVector(), generator.rand));
	}
	
	prepareHeightmap();
	//prepareBorderHeightmap();
	
	rmg::Area outOfMapTiles;
	std::map<TRmgTemplateZoneId, rmg::Area> neighbourZonesTiles;
	rmg::Area borderArea(zone.getArea().getBorder());
	for(auto & t : zone.getArea().getBorderOutside())
	{
		if(!map.isOnMap(t))
		{
			outOfMapTiles.add(t);
		}
		else if(map.getZoneID(t) != zone.getId())
		{
			neighbourZonesTiles[map.getZoneID(t)].add(t);
		}
	}
	rmg::Area outOfMapInternal(outOfMapTiles.getBorderOutside());
	outOfMapInternal.intersect(borderArea);
	
	//looking outside map
	if(!outOfMapTiles.empty())
	{
		auto elem = *RandomGeneratorUtil::nextItem(outOfMapInternal.getTilesVector(), generator.rand);
		source.add(elem);
		outOfMapInternal.erase(elem);
	}
	if(!outOfMapTiles.empty())
	{
		auto elem = *RandomGeneratorUtil::nextItem(outOfMapInternal.getTilesVector(), generator.rand);
		sink.add(elem);
		outOfMapInternal.erase(elem);
	}
	
	
	if(source.empty())
	{
		logGlobal->info("River source is empty!");
		
		//looking outside map
		
		for(auto & i : heightMap)
		{
			if(i.second > 0)
				source.add(i.first);
		}
	}
	
	if(sink.empty())
	{
		logGlobal->error("River sink is empty!");
		for(auto & i : heightMap)
		{
			if(i.second <= 0)
				sink.add(i.first);
		}
	}
}

void RiverPlacer::connectRiver(const int3 & tile)
{
	if(source.contains(tile) || sink.contains(tile))
		return;
	
	auto movementCost = [this](const int3 & s, const int3 & d)
	{
		float cost = 1.0f;

		if(!zone.areaPossible().contains(d))
			cost += 2.f;
		
		cost += heightMap[d];
		return cost;
	};
	
	rmg::Path pathToSource(zone.area() - rivers);
	pathToSource.connect(source);
	pathToSource = pathToSource.search(tile, true, movementCost);
	
	rmg::Path pathToSink(zone.area() - pathToSource.getPathArea());
	pathToSink.connect(sink);
	pathToSink = pathToSink.search(tile, true, movementCost);
	
	if(pathToSource.getPathArea().empty() || pathToSink.getPathArea().empty())
	{
		logGlobal->error("Cannot build river");
		return;
	}
	
	rivers.unite(pathToSource.getPathArea());
	rivers.unite(pathToSink.getPathArea());
	sink.unite(rivers);
}
