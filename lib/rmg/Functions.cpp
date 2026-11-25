/*
 * Functions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Functions.h"
#include "CMapGenerator.h"
#include "ObjectInfo.h"
#include "RmgMap.h"
#include "TileInfo.h"
#include "RmgPath.h"
#include "modificators/RoadPlacer.h"
#include "../TerrainHandler.h"
#include "../mapping/CMap.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../GameLibrary.h"
#include "../CCreatureHandler.h"
#include "../entities/faction/CFaction.h"
#include "../entities/faction/CTown.h"
#include "../mapObjects/MiscObjects.h"

#include <vstd/RNG.h>

rmg::BezierCurve chooseCurve(Zone & zone, const int3 & src)
{
	auto & rng = zone.getRand();
	const auto & tiles = zone.area()->getTilesVector();
	int3 control1 = tiles[rng.nextInt(0, static_cast<int>(tiles.size()) - 1)];
	int3 control2 = tiles[rng.nextInt(0, static_cast<int>(tiles.size()) - 1)];
	return {src, control1, control2, zone.getPos()};
}

void replaceWithCurvedPath(rmg::Path & path, Zone & zone, const int3 & src, bool onlyStraight /* = true */)
{
	if(zone.area()->getTilesVector().size() < 2)
	{
		logRmg->warn("Zone too small for curved path");
		return;
	}

	const auto curve = chooseCurve(zone, src);

	auto pathArea = zone.areaForRoads();
	rmg::Path curvedPath(pathArea);
	curvedPath.connect(zone.freePaths().get());
	curvedPath = curvedPath.search(src, onlyStraight, curve.costFunction());
	if (curvedPath.valid())
	{
		path = curvedPath;
		if(auto * rp = zone.getModificator<RoadPlacer>())
			rp->setNodeCurve(src, curve);
	}
	else
	{
		logRmg->warn("Failed to create curved path to %s", src.toString());
	}
}

namespace
{
// 4-adjacent roads, plus a diagonal when the two tiles share no 4-adjacent road tile.
int roadEndDegree(const rmg::Area & roads, const int3 & tile)
{
	int degree = 0;
	for(const auto & dir : rmg::dirs4)
	{
		if(roads.contains(tile + dir))
			++degree;
	}
	for(const auto & dir : rmg::dirsDiagonal)
	{
		const int3 next = tile + dir;
		if(!roads.contains(next))
			continue;
		const int3 sharedX = tile + int3(dir.x, 0, 0);
		const int3 sharedY = tile + int3(0, dir.y, 0);
		if(!roads.contains(sharedX) && !roads.contains(sharedY))
			++degree;
	}
	return degree;
}
}

rmg::Area pruneRoadDeadEnds(const rmg::Area & roads, const rmg::Tileset & keep)
{
	rmg::Area remaining = roads;
	bool changed = true;
	while(changed)
	{
		changed = false;
		std::vector<int3> toRemove;
		for(const int3 & tile : remaining.getTiles())
		{
			if(keep.contains(tile))
				continue;
			if(roadEndDegree(remaining, tile) <= 1)
				toRemove.push_back(tile);
		}

		for(const int3 & tile : toRemove)
		{
			remaining.erase(tile);
			changed = true;
		}
	}
	return remaining;
}

rmg::Tileset collectDistantTiles(const Zone& zone, int distance)
{
	uint32_t distanceSq = distance * distance;

	auto subarea = zone.area()->getSubarea([&zone, distanceSq](const int3 & t)
	{
		return t.dist2dSQ(zone.getPos()) > distanceSq;
	});
	return subarea.getTiles();
}

int chooseRandomAppearance(vstd::RNG & generator, si32 ObjID, TerrainId terrain)
{
	auto factories = LIBRARY->objtypeh->knownSubObjects(ObjID);
	vstd::erase_if(factories, [ObjID, &terrain](si32 f)
	{
		//TODO: Use templates with lowest number of terrains (most specific)
		return LIBRARY->objtypeh->getHandlerFor(ObjID, f).getTemplates(terrain).empty();
	});
	
	return *RandomGeneratorUtil::nextItem(factories, generator);
}

float getNativeDwellingValueModifier(RmgMap & map, FactionID faction)
{
	float nativeZonesCount = static_cast<float>(map.getZoneCount(faction));
	// value increases, if there are more native zones for the faction
	return 1 + (nativeZonesCount / map.getTotalZoneCount()) + (nativeZonesCount / 2);
}

ui32 getNativeDwellingValue(RmgMap & map, const CCreature * creature)
{
	return static_cast<ui32>(creature->getAIValue() * creature->getGrowth() * getNativeDwellingValueModifier(map, creature->getFactionID()));
}

const CCreature * getRandomDwellingCreature(FactionID faction, int level)
{
	if(faction == FactionID::NEUTRAL || level < 0)
		return nullptr;

	const auto * factionPtr = faction.toFaction();
	if(!factionPtr || !factionPtr->getTown())
		return nullptr;

	const auto & tiers = factionPtr->getTown()->creatures;
	if(static_cast<size_t>(level) >= tiers.size() || tiers[level].empty())
		return nullptr;

	return tiers[level].front().toCreature();
}

ui32 getRandomDwellingValue(RmgMap & map, FactionID faction, int level)
{
	const auto * creature = getRandomDwellingCreature(faction, level);
	if(!creature)
		return 0;

	return getNativeDwellingValue(map, creature);
}

std::vector<int> waterZoneLevels(EWaterContent::EWaterContent water,
	const std::vector<MapLayerId> & levelLayers, const std::set<int> & landZoneLevels)
{
	if(water == EWaterContent::NONE)
		return {};

	std::vector<int> result;
	for(int level : landZoneLevels)
	{
		if(static_cast<size_t>(level) >= levelLayers.size())
			continue;
		if(levelLayers[level] != MapLayerId::UNDERGROUND)
			result.push_back(level);
	}
	return result;
}

bool footprintKeepsDistance(const rmg::Area & footprint, const rmg::Area & placed, float minDistance)
{
	if(placed.empty())
		return true;

	return std::ranges::all_of(footprint.getTilesVector(), [&placed, minDistance](const int3 & tile)
	{
		return static_cast<float>(placed.distanceSqr(tile)) >= minDistance;
	});
}

bool canJoinTreasurePile(const ObjectInfo & candidate, const std::vector<ObjectInfo *> & pile)
{
	if(candidate.isStandalone() && !pile.empty())
		return false;

	return std::ranges::none_of(pile, [](const ObjectInfo * existing)
	{
		return existing->isStandalone();
	});
}

std::shared_ptr<CGObjectInstance> createSpellScroll(RmgMap & map, SpellID spell)
{
	const auto & factory = LIBRARY->objtypeh->getHandlerFor(Obj::SPELL_SCROLL, 0);
	auto obj = std::dynamic_pointer_cast<CGArtifact>(factory.create(map.mapInstance->cb, nullptr));
	const auto * a = map.getMapProxy()->createScroll(spell);
	obj->setArtifactInstance(a);
	return obj;
}
