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
	prepareBorderHeightmap();
	
	if(source.empty())
	{
		logGlobal->error("River source is empty!");
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
	assert(!source.contains(tile));
	assert(!sink.contains(tile));
	
	rmg::Path path1(zone.area()), path2(zone.area());
	path1.connect(source);
	path2.connect(sink);
	path1 = path1.search(tile, true);
	path2 = path2.search(tile, true);
	if(path1.getPathArea().empty() || path2.getPathArea().empty())
	{
		logGlobal->error("Cannot build river");
		return;
	}
	
	rivers.unite(path1.getPathArea());
	rivers.unite(path2.getPathArea());
	sink.unite(rivers);
}
