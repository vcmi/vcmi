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
#include "../entities/faction/CTownHandler.h"
#include "../mapObjects/MiscObjects.h"

#include <vstd/RNG.h>

#include <set>
#include <string>

rmg::BezierCurve chooseCurve(const Zone & zone, const int3 & src, const rmg::DistanceMap & borderDistance, vstd::RNG & rng)
{
	constexpr int MIN_RING = 4; // reject a curve that comes this close to the zone border
	constexpr int MAX_TRIES = 10;

	const auto & tiles = zone.area()->getTilesVector();
	rmg::BezierCurve best;
	int bestRing = std::numeric_limits<int>::min();

	for(int attempt = 0; attempt < MAX_TRIES; ++attempt)
	{
		const int3 control1 = tiles[rng.nextInt(0, static_cast<int>(tiles.size()) - 1)];
		const int3 control2 = tiles[rng.nextInt(0, static_cast<int>(tiles.size()) - 1)];
		const rmg::BezierCurve candidate{src, control1, control2, zone.getPos()};

		const int ring = candidate.minSampledRing(borderDistance, MIN_RING);
		if(ring >= MIN_RING)
			return candidate;

		if(ring > bestRing)
		{
			best = candidate;
			bestRing = ring;
		}
	}

	logRmg->debug("Zone %d: no Bezier curve stayed inside", zone.getId());
	return best;
}

void replaceWithCurvedPath(rmg::Path & path, Zone & zone, const int3 & src, bool onlyStraight /* = true */)
{
	if(zone.area()->getTilesVector().size() < 2)
	{
		logRmg->warn("Zone too small for curved path");
		return;
	}

	const auto borderDistance = zone.area()->computeDistanceMap();
	const auto curve = chooseCurve(zone, src, borderDistance, zone.getRand());

	auto pathArea = zone.areaForRoads();
	rmg::Path curvedPath(pathArea);
	curvedPath.connect(zone.freePaths().get());
	curvedPath = curvedPath.search(src, onlyStraight, curve.costFunction(borderDistance));
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

namespace
{
// Road sprites join only orthogonal neighbours, so a road network is connected in 4 directions.
int countRoadPieces(const rmg::Area & roads)
{
	rmg::Tileset unvisited = roads.getTiles();
	int pieces = 0;
	while(!unvisited.empty())
	{
		++pieces;
		std::vector<int3> queue = {*unvisited.begin()};
		unvisited.erase(unvisited.begin());
		while(!queue.empty())
		{
			const int3 tile = queue.back();
			queue.pop_back();
			for(const auto & dir : rmg::dirs4)
			{
				if(unvisited.erase(tile + dir))
					queue.push_back(tile + dir);
			}
		}
	}
	return pieces;
}

// A turn whose two neighbours continue the staircase is drawn as a diagonal road piece.
bool isStaircaseTile(const rmg::Area & roads, const int3 & tile)
{
	std::vector<int3> neighbours;
	for(const auto & dir : rmg::dirs4)
	{
		if(roads.contains(tile + dir))
			neighbours.push_back(dir);
	}
	if(neighbours.size() != 2 || neighbours[0] + neighbours[1] == int3())
		return false;
	const int3 & a = neighbours[0];
	const int3 & b = neighbours[1];
	return roads.contains(tile + a - b) && roads.contains(tile + b - a);
}

int countStaircaseTiles(const rmg::Area & roads, const int3 & topLeft, int radius)
{
	int count = 0;
	for(int y = topLeft.y - radius; y <= topLeft.y + 1 + radius; ++y)
	{
		for(int x = topLeft.x - radius; x <= topLeft.x + 1 + radius; ++x)
		{
			const int3 tile(x, y, topLeft.z);
			if(roads.contains(tile) && isStaircaseTile(roads, tile))
				++count;
		}
	}
	return count;
}
}

rmg::Area removeRoadSquares(const rmg::Area & roads, const rmg::Tileset & keep)
{
	constexpr int SCORE_RADIUS = 2;
	rmg::Area remaining = roads;
	const int pieces = countRoadPieces(remaining);
	bool changed = true;
	while(changed)
	{
		changed = false;
		std::vector<int3> corners = remaining.getTilesVector();
		std::sort(corners.begin(), corners.end());
		for(const int3 & corner : corners)
		{
			const std::array<int3, 4> square = {corner, corner + int3(1, 0, 0), corner + int3(0, 1, 0), corner + int3(1, 1, 0)};
			if(!std::all_of(square.begin(), square.end(), [&remaining](const int3 & t) { return remaining.contains(t); }))
				continue;

			std::optional<int3> best;
			int bestScore = -1;
			for(const int3 & tile : square)
			{
				if(keep.contains(tile))
					continue;
				rmg::Area candidate = remaining;
				candidate.erase(tile);
				if(countRoadPieces(candidate) != pieces)
					continue;
				const int score = countStaircaseTiles(candidate, corner, SCORE_RADIUS);
				if(score > bestScore)
				{
					best = tile;
					bestScore = score;
				}
			}
			if(best)
			{
				remaining.erase(*best);
				changed = true;
			}
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

bool factionHasTown(FactionID faction)
{
	if(!faction.isValid())
		return false;
	const auto * f = faction.toFaction();
	return f && f->hasTown();
}

FactionID townFactionOrRandom(FactionID hinted, const std::set<FactionID> & allowed, vstd::RNG & rand)
{
	if(factionHasTown(hinted))
		return hinted;

	std::set<FactionID> pool;
	for(const auto & faction : allowed)
	{
		if(factionHasTown(faction))
			pool.insert(faction);
	}

	if(pool.empty())
		pool = LIBRARY->townh->getDefaultAllowed();

	return *RandomGeneratorUtil::nextItem(pool, rand);
}

namespace
{
std::vector<TRmgTemplateZoneId> firstHintDependencies(const std::vector<rmg::ZoneOptions::CTownHints> & hints)
{
	if(hints.empty())
		return {};

	const auto & hint = hints.front();
	if(hint.likeZone != rmg::ZoneOptions::NO_ZONE)
		return {hint.likeZone};
	if(!hint.notLikeZone.empty())
		return hint.notLikeZone;
	if(hint.relatedToZoneTerrain != rmg::ZoneOptions::NO_ZONE)
		return {hint.relatedToZoneTerrain};
	return {};
}
}

std::vector<TRmgTemplateZoneId> orderZonesByFirstTownHint(const std::map<TRmgTemplateZoneId, std::vector<rmg::ZoneOptions::CTownHints>> & townHints)
{
	std::map<TRmgTemplateZoneId, std::vector<TRmgTemplateZoneId>> dependents;
	std::map<TRmgTemplateZoneId, int> indegree;
	for(const auto & [zoneId, _] : townHints)
		indegree[zoneId] = 0;

	for(const auto & [zoneId, hints] : townHints)
	{
		std::set<TRmgTemplateZoneId> uniqueDeps;
		for(const auto dep : firstHintDependencies(hints))
		{
			if(dep == zoneId || !townHints.contains(dep))
				continue;
			uniqueDeps.insert(dep);
		}
		for(const auto dep : uniqueDeps)
		{
			dependents[dep].push_back(zoneId);
			++indegree[zoneId];
		}
	}

	std::set<TRmgTemplateZoneId> ready;
	for(const auto & [zoneId, degree] : indegree)
	{
		if(degree == 0)
			ready.insert(zoneId);
	}

	std::vector<TRmgTemplateZoneId> order;
	order.reserve(townHints.size());
	while(!ready.empty())
	{
		const auto zoneId = *ready.begin();
		ready.erase(ready.begin());
		order.push_back(zoneId);
		for(const auto next : dependents[zoneId])
		{
			if(--indegree[next] == 0)
				ready.insert(next);
		}
	}

	if(order.size() == townHints.size())
		return order;

	std::string message = "Town hints form a dependency cycle between zones ";
	bool first = true;
	for(const auto & [zoneId, degree] : indegree)
	{
		if(degree <= 0)
			continue;
		if(!first)
			message += ", ";
		first = false;
		message += std::to_string(zoneId);
	}
	throw rmgException(message);
}

CreatureID chooseFallbackGuard(si32 strength, const std::vector<GuardCandidate> & candidates)
{
	if(candidates.empty())
		return CreatureID::PEASANT;

	auto distanceToWindow = [strength](const GuardCandidate & candidate) -> si64
	{
		if(strength < candidate.minStrength)
			return static_cast<si64>(candidate.minStrength) - strength;
		if(strength > candidate.maxStrength)
			return static_cast<si64>(strength) - candidate.maxStrength;
		return 0;
	};

	CreatureID best = candidates.front().id;
	si64 bestDistance = distanceToWindow(candidates.front());
	for(size_t i = 1; i < candidates.size(); ++i)
	{
		const si64 distance = distanceToWindow(candidates[i]);
		if(distance < bestDistance)
		{
			bestDistance = distance;
			best = candidates[i].id;
		}
	}
	return best;
}

si32 guardStackSize(si32 strength, si32 aiValue)
{
	return std::max(1, strength / aiValue);
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
