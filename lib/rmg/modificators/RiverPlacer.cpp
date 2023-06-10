/*
 * RiverPlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "RiverPlacer.h"
#include "../Functions.h"
#include "../CMapGenerator.h"
#include "../RmgMap.h"
#include "../../RiverHandler.h"
#include "../../TerrainHandler.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapObjects/ObjectTemplate.h"
#include "../../mapping/CMap.h"
#include "../../mapping/CMapEditManager.h"
#include "../RmgPath.h"
#include "ObjectManager.h"
#include "ObstaclePlacer.h"
#include "WaterProxy.h"
#include "RoadPlacer.h"

VCMI_LIB_NAMESPACE_BEGIN

const int RIVER_DELTA_ID = 143;
const int RIVER_DELTA_SUBTYPE = 0;

const std::array<std::array<int, 25>, 4> deltaTemplates
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
	for(const auto & t : riverNodes)
		connectRiver(t);
	
	if(!rivers.empty())
		drawRivers();
}

void RiverPlacer::init()
{
	if (!zone.isUnderground())
	{
		DEPENDENCY_ALL(WaterProxy);
	}
	DEPENDENCY(ObjectManager);
	DEPENDENCY(ObstaclePlacer);
}

void RiverPlacer::drawRivers()
{
	auto tiles = rivers.getTilesVector();
	mapProxy->drawRivers(zone.getRand(), tiles, zone.getTerrainType());
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

rmg::Area & RiverPlacer::riverProhibit()
{
	return prohibit;
}

void RiverPlacer::prepareHeightmap()
{
	rmg::Area roads;
	if(auto * m = zone.getModificator<RoadPlacer>())
	{
		roads.unite(m->getRoads());
	}

	for(const auto & t : zone.area().getTilesVector())
	{
		heightMap[t] = zone.getRand().nextInt(5);
		
		if(roads.contains(t))
			heightMap[t] += 30.f;
		
		if(zone.areaUsed().contains(t))
			heightMap[t] += 1000.f;
	}
	
	//make grid
	for(int j = 0; j < map.height(); j += 2)
	{
		for(int i = 0; i < map.width(); i += 2)
		{
			int3 t{i, j, zone.getPos().z};
			if(zone.area().contains(t))
				heightMap[t] += 10.f;
		}
	}
}

void RiverPlacer::preprocess()
{
	rmg::Area outOfMapTiles;
	std::map<TRmgTemplateZoneId, rmg::Area> neighbourZonesTiles;
	rmg::Area borderArea(zone.getArea().getBorder());
	TRmgTemplateZoneId connectedToWaterZoneId = -1;
	for(const auto & t : zone.getArea().getBorderOutside())
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
	if(!outOfMapInternal.empty())
	{
		auto elem = *RandomGeneratorUtil::nextItem(outOfMapInternal.getTilesVector(), zone.getRand());
		source.add(elem);
		outOfMapInternal.erase(elem);
	}
	if(!outOfMapInternal.empty())
	{
		auto elem = *RandomGeneratorUtil::nextItem(outOfMapInternal.getTilesVector(), zone.getRand());
		sink.add(elem);
		outOfMapInternal.erase(elem);
	}
	
	//calculate delta positions
	if(connectedToWaterZoneId > -1)
	{
		auto river = VLC->terrainTypeHandler->getById(zone.getTerrainType())->river;
		auto & a = neighbourZonesTiles[connectedToWaterZoneId];
		auto availableArea = zone.areaPossible() + zone.freePaths();
		for(const auto & tileToProcess : availableArea.getTilesVector())
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
						deltaOrientations[p] = templateId + 1;
						
						//specific case: deltas for ice rivers amd mud rivers are messed :(
						if(river == River::ICY_RIVER)
						{
							switch(deltaOrientations[p])
							{
								case 1:
									deltaOrientations[p] = 2;
									break;
								case 2:
									deltaOrientations[p] = 3;
									break;
								case 3:
									deltaOrientations[p] = 4;
									break;
								case 4:
									deltaOrientations[p] = 1;
									break;
							}
						}
						if(river == River::MUD_RIVER)
						{
							switch(deltaOrientations[p])
							{
								case 1:
									deltaOrientations[p] = 4;
									break;
								case 2:
									deltaOrientations[p] = 3;
									break;
								case 3:
									deltaOrientations[p] = 1;
									break;
								case 4:
									deltaOrientations[p] = 2;
									break;
							}
						}
						
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
		addRiverNode(*RandomGeneratorUtil::nextItem(source.getTilesVector(), zone.getRand()));
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
	auto riverType = VLC->terrainTypeHandler->getById(zone.getTerrainType())->river;
	const auto * river = VLC->riverTypeHandler->getById(riverType);
	if(river->getId() == River::NO_RIVER)
		return;
	
	rmg::Area roads;
	if(auto * m = zone.getModificator<RoadPlacer>())
	{
		roads.unite(m->getRoads());
	}
	
	auto movementCost = [this, &roads](const int3 & s, const int3 & d)
	{
		float cost = heightMap[d];
		if(roads.contains(s))
			cost += 1000.f; //allow road intersection, but avoid long overlaps
		return cost;
	};
	
	auto availableArea = zone.area() - prohibit;
	
	rmg::Path pathToSource(availableArea);
	pathToSource.connect(source);
	pathToSource.connect(rivers);
	pathToSource = pathToSource.search(tile, true, movementCost);
	
	availableArea.subtract(pathToSource.getPathArea());
	
	rmg::Path pathToSink(availableArea);
	pathToSink.connect(sink);
	pathToSource.connect(rivers);
	pathToSink = pathToSink.search(tile, true, movementCost);
	
	if(pathToSource.getPathArea().empty() || pathToSink.getPathArea().empty())
	{
		logGlobal->error("Cannot build river");
		return;
	}
	
	//delta placement
	auto deltaPos = pathToSink.getPathArea() * deltaSink;
	if(!deltaPos.empty())
	{
		assert(deltaPos.getTilesVector().size() == 1);
		
		auto pos = deltaPos.getTilesVector().front();
		auto handler = VLC->objtypeh->getHandlerFor(RIVER_DELTA_ID, RIVER_DELTA_SUBTYPE);
		assert(handler->isStaticObject());
		
		std::vector<std::shared_ptr<const ObjectTemplate>> tmplates;
		for(auto & temp : handler->getTemplates())
		{
			if(temp->canBePlacedAt(zone.getTerrainType()))
			   tmplates.push_back(temp);
		}
		
		if(tmplates.size() > 3)
		{
			if(tmplates.size() % 4 != 0)
				throw rmgException(boost::to_string(boost::format("River templates for (%d,%d) at terrain %s, river %s are incorrect") %
					RIVER_DELTA_ID % RIVER_DELTA_SUBTYPE % zone.getTerrainType() % river->shortIdentifier));
			
			std::string targetTemplateName = river->deltaName + std::to_string(deltaOrientations[pos]) + ".def";
			for(auto & templ : tmplates)
			{
				if(templ->animationFile == targetTemplateName)
				{
					auto * obj = handler->create(templ);
					rmg::Object deltaObj(*obj, deltaPositions[pos]);
					deltaObj.finalize(map);
				}
			}
		}
	}
	
	rivers.unite(pathToSource.getPathArea());
	rivers.unite(pathToSink.getPathArea());
}

VCMI_LIB_NAMESPACE_END
