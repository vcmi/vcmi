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
#include "../mapObjects/CObjectClassesHandler.h"
#include "RmgPath.h"
#include "ObjectManager.h"
#include "ObstaclePlacer.h"
#include "WaterProxy.h"
#include "RoadPlacer.h"

std::array<std::array<int, 25>, 4> deltaTemplates
{
	//0 - must be on ground
	//1 - delta entry
	//2 - must be on water
	//3 - anything
	//4 - prohibit river placement
	//5 - must be on ground + position
	//6 - must be on water + position
	std::array<int, 25>{
		3, 4, 3, 4, 3,
		3, 4, 1, 4, 3,
		3, 0, 0, 0, 3,
		3, 0, 0, 0, 3,
		3, 2, 2, 6, 3
	},
	std::array<int, 25>{
		3, 2, 2, 2, 3,
		3, 0, 0, 0, 3,
		3, 0, 0, 5, 3,
		3, 4, 1, 4, 3,
		3, 4, 3, 4, 3
	},
	std::array<int, 25>{
		3, 3, 3, 3, 3,
		4, 4, 0, 0, 2,
		3, 1, 0, 0, 2,
		4, 4, 0, 0, 6,
		3, 3, 3, 3, 3
	},
	std::array<int, 25>	{
		3, 3, 3, 3, 3,
		2, 0, 0, 4, 4,
		2, 0, 0, 1, 3,
		2, 0, 5, 4, 4,
		3, 3, 3, 3, 3
	}
};

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
		return '2';
	if(source.contains(t))
		return '1';
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
	rmg::Area roads;
	if(auto * m = zone.getModificator<RoadPlacer>())
	{
		roads.unite(m->getRoads());
	}
	
	for(auto & t : zone.area().getTilesVector())
	{
		heightMap[t] = generator.rand.nextInt(5);
		
		if(roads.contains(t))
			heightMap[t] += 30.f;
		
		if(zone.areaUsed().contains(t))
			heightMap[t] += 1000.f;
	}
	
	//make grid
	for(int j = 0; j < map.map().height; j += 2)
	{
		for(int i = 0; i < map.map().width; i += 2)
		{
			int3 t{i, j, zone.getPos().z};
			if(zone.area().contains(t))
				heightMap[t] += 10.f;
		}
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
	rmg::Area outOfMapTiles;
	std::map<TRmgTemplateZoneId, rmg::Area> neighbourZonesTiles;
	rmg::Area borderArea(zone.getArea().getBorder());
	TRmgTemplateZoneId connectedToWaterZoneId = -1;
	for(auto & t : zone.getArea().getBorderOutside())
	{
		if(!map.isOnMap(t))
		{
			outOfMapTiles.add(t);
		}
		else if(map.getZoneID(t) != zone.getId())
		{
			if(map.getZones()[map.getZoneID(t)]->getType() == ETemplateZoneType::WATER)
				connectedToWaterZoneId = map.getZoneID(t);
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
	
	//calculate delta positions
	if(connectedToWaterZoneId > -1)
	{
		auto & a = neighbourZonesTiles[connectedToWaterZoneId];
		auto availableArea = zone.areaPossible() + zone.freePaths();
		for(auto & tileToProcess : availableArea.getTilesVector())
		{
			int templateId = -1;
			for(int tId = 0; tId < 4; ++tId)
			{
				templateId = tId;
				for(int i = 0; i < 25; ++i)
				{
					if((deltaTemplates[tId][i] == 2 || deltaTemplates[tId][i] == 6) && !a.contains(tileToProcess + int3(i % 5 - 2, i / 5 - 2, 0)))
					{
						templateId = -1;
						break;
					}
					if((deltaTemplates[tId][i] < 2 || deltaTemplates[tId][i] == 5) && !availableArea.contains(tileToProcess + int3(i % 5 - 2, i / 5 - 2, 0)))
					{
						templateId = -1;
						break;
					}
				}
				if(templateId > -1)
					break;
			}
			
			if(templateId > -1)
			{
				for(int i = 0; i < 25; ++i)
				{
					auto p = tileToProcess + int3(i % 5 - 2, i / 5 - 2, 0);
					if(deltaTemplates[templateId][i] == 1)
					{
						sink.add(p);
						deltaSink.add(p);
						deltaOrientations[p] = templateId;
						for(auto j = 0; j < 25; ++j)
						{
							if(deltaTemplates[templateId][j] >= 5)
							{
								deltaPositions[p] = tileToProcess + int3(j % 5 - 2, j / 5 - 2, 0);
							}
						}
					}
					if(deltaTemplates[templateId][i] == 0 || deltaTemplates[templateId][i] == 4 || deltaTemplates[templateId][i] == 5)
					{
						prohibit.add(p);
					}
				}
			}
		}
	}
	
	prepareHeightmap();
	
	//decorative river
	if(!sink.empty() && !source.empty() && riverNodes.empty() && !zone.areaPossible().empty())
	{
		addRiverNode(*RandomGeneratorUtil::nextItem(source.getTilesVector(), generator.rand));
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
	//if(source.contains(tile) || sink.contains(tile))
	//	return;
	
	auto movementCost = [this](const int3 & s, const int3 & d, const rmg::Area & avoid)
	{
		float cost = heightMap[d];
		
		//cost -= sqrt(avoid.distanceSqr(d));
		
		return cost;
	};
	
	auto availableArea = zone.area() - prohibit;
	auto searchAreaTosource = availableArea;
	
	rmg::Path pathToSource(searchAreaTosource);
	pathToSource.connect(source);
	pathToSource = pathToSource.search(tile, true, std::bind(movementCost, std::placeholders::_1, std::placeholders::_2, sink));
	
	auto searchAreaToSink = availableArea - pathToSource.getPathArea();
	
	rmg::Path pathToSink(searchAreaToSink);
	pathToSink.connect(sink);
	pathToSink = pathToSink.search(tile, true, std::bind(movementCost, std::placeholders::_1, std::placeholders::_2, pathToSource.getPathArea()));
	
	if(pathToSource.getPathArea().empty() || pathToSink.getPathArea().empty())
	{
		logGlobal->error("Cannot build river");
		return;
	}
	
	//delta placement
	auto deltaPos = pathToSink.getPathArea() * deltaSink;
	if(!deltaPos.empty())
	{
		const int RIVER_DELTA_ID = 143;
		const int RIVER_DELTA_SUBTYPE = 0;
		assert(deltaPos.getTilesVector().size() == 1);
		
		auto pos = deltaPos.getTilesVector().front();
		
		auto handler = VLC->objtypeh->getHandlerFor(RIVER_DELTA_ID, RIVER_DELTA_SUBTYPE);
		assert(handler->isStaticObject());
		
		std::vector<ObjectTemplate> tmplates;
		for(auto & temp : handler->getTemplates())
		{
			if(temp.canBePlacedAt(zone.getTerrainType()))
			   tmplates.push_back(temp);
		}
		
		if(tmplates.size() > 3)
		{
			if(tmplates.size() % 4 != 0)
				throw rmgException(boost::to_string(boost::format("River templates for (%d,%d) at terrain %s are incorrect") % RIVER_DELTA_ID % RIVER_DELTA_SUBTYPE % zone.getTerrainType()));
			
			auto obj = handler->create(tmplates[deltaOrientations[pos]]);
			rmg::Object deltaObj(*obj, deltaPositions[pos]);
			deltaObj.finalize(map);
		}
	}
	
	rivers.unite(pathToSource.getPathArea());
	rivers.unite(pathToSink.getPathArea());
	sink.add(tile);
}
