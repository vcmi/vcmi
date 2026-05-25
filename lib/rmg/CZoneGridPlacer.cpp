/*
 * CZoneGridPlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CZoneGridPlacer.h"

#include "CMapGenOptions.h"
#include "Functions.h"
#include "RmgMap.h"
#include "Zone.h"

#include <vstd/RNG.h>

#include <limits>

VCMI_LIB_NAMESPACE_BEGIN

namespace
{
template <typename Fn>
void forEachInGrid(const size_t gridSize, Fn && fn)
{
	for (size_t x = 0; x < gridSize; ++x)
		for (size_t y = 0; y < gridSize; ++y)
			fn(x, y);
}
}

CZoneGridPlacer::CZoneGridPlacer(const RmgMap & map, const DistanceMap & distancesBetweenZones, ScaleForceFn scaleForceBetweenZones)
	: map(map)
	, distancesBetweenZones(distancesBetweenZones)
	, scaleForceBetweenZones(std::move(scaleForceBetweenZones))
{
}

CZoneGridPlacer::GridType & CZoneGridPlacer::getGridForLevel(std::vector<std::unique_ptr<GridType>> & grids, int level) const
{
	return *grids.at(level);
}

void CZoneGridPlacer::getRandomGridCorner(vstd::RNG * rand, size_t gridSize, size_t & x, size_t & y)
{
	if (rand->nextInt(0, 1) == 1)
		x = 0;
	else
		x = gridSize - 1;
	if (rand->nextInt(0, 1) == 1)
		y = 0;
	else
		y = gridSize - 1;
}

void CZoneGridPlacer::getRandomGridEdge(vstd::RNG * rand, size_t gridSize, size_t & x, size_t & y)
{
	switch (rand->nextInt(0, 3) % 4)
	{
	case 0:
		x = 0;
		y = gridSize / 2;
		break;
	case 1:
		x = gridSize - 1;
		y = gridSize / 2;
		break;
	case 2:
		x = gridSize / 2;
		y = 0;
		break;
	case 3:
		x = gridSize / 2;
		y = gridSize - 1;
		break;
	}
}

bool CZoneGridPlacer::isWithinGrid(const int3 & cell, size_t gridSize)
{
	const auto n = static_cast<si32>(gridSize);
	return cell.x >= 0 && cell.x < n && cell.y >= 0 && cell.y < n;
}

bool CZoneGridPlacer::betterByPrimaryThenTie(
	bool maximizePrimary,
	float candidatePrimary,
	float bestPrimary,
	float candidateTie,
	float bestTie)
{
	constexpr float primaryEpsilon = 1e-4f;
	if (maximizePrimary)
	{
		const bool betterPrimary = candidatePrimary > bestPrimary + primaryEpsilon;
		const bool samePrimary = std::abs(candidatePrimary - bestPrimary) <= primaryEpsilon;
		return betterPrimary || (samePrimary && candidateTie > bestTie);
	}
	const bool betterPrimary = candidatePrimary < bestPrimary - primaryEpsilon;
	const bool samePrimary = std::abs(candidatePrimary - bestPrimary) <= primaryEpsilon;
	return betterPrimary || (samePrimary && candidateTie > bestTie);
}

std::vector<std::unique_ptr<CZoneGridPlacer::GridType>> CZoneGridPlacer::createGrids(
	const ZoneMap & zones,
	int mapLevels,
	std::vector<size_t> & gridSizes) const
{
	std::vector<std::unique_ptr<GridType>> grids(mapLevels);
	gridSizes.assign(mapLevels, 0);

	std::vector<int> zonesPerLevel(mapLevels, 0);
	for (const auto & zoneEntry : zones)
	{
		int level = zoneEntry.second->getCenter().z;
		if (level >= 0 && level < mapLevels)
			zonesPerLevel[level]++;
	}

	for (int level = 0; level < mapLevels; ++level)
	{
		if (zonesPerLevel[level] <= 0)
			continue;

		const size_t size = std::ceil(std::sqrt(zonesPerLevel[level]));
		gridSizes[level] = size;
		grids[level] = std::make_unique<GridType>(boost::extents[size][size]);
	}

	return grids;
}

std::vector<std::shared_ptr<Zone>> CZoneGridPlacer::findAnchors(
	const std::shared_ptr<Zone> & zone,
	const ZoneMap & zones,
	const std::map<TRmgTemplateZoneId, int3> & placedPositions) const
{
	std::vector<std::shared_ptr<Zone>> anchors;
	for (const auto & conn : zone->getConnections())
	{
		if (conn.getConnectionType() == rmg::EConnectionType::REPULSIVE ||
			conn.getConnectionType() == rmg::EConnectionType::FORCE_PORTAL)
		{
			continue;
		}

		auto otherId = conn.getOtherZoneId(zone->getId());
		if (placedPositions.count(otherId))
			anchors.push_back(zones.at(otherId));
	}

	return anchors;
}

float CZoneGridPlacer::calculateUnconnectedTieBreak(
	const int3 & gridPos,
	const std::shared_ptr<Zone> & zone,
	const GridType & grid,
	size_t gridSize) const
{
	float acc = 0.f;
	forEachInGrid(gridSize, [this, &acc, &grid, &gridPos, &zone](size_t existingX, size_t existingY)
	{
		const auto existingZone = grid[existingX][existingY];
		if (!existingZone)
			return;

		size_t graphDist = 0;
		auto outerIt = distancesBetweenZones.find(zone->getId());
		if (outerIt != distancesBetweenZones.end())
		{
			auto innerIt = outerIt->second.find(existingZone->getId());
			if (innerIt != outerIt->second.end())
				graphDist = innerIt->second;
		}
		if (graphDist == 1)
			return;
		if (graphDist == 0)
			graphDist = 32; // different graph components: emphasize grid distance

		const auto d2 = static_cast<float>(gridPos.dist2d(int3(static_cast<si32>(existingX), static_cast<si32>(existingY), 0)));
		acc += d2 * static_cast<float>(graphDist) * scaleForceBetweenZones(zone, existingZone);
	});
	return acc;
}

float CZoneGridPlacer::sumWeightedDistanceToPlacedZones(
	const GridType & grid,
	size_t gridSize,
	const std::shared_ptr<Zone> & zone,
	size_t freeX,
	size_t freeY) const
{
	const int3 potentialPos(static_cast<si32>(freeX), static_cast<si32>(freeY), 0);
	float distance = 0;
	forEachInGrid(gridSize, [this, &grid, &potentialPos, &zone, &distance](size_t existingX, size_t existingY)
	{
		const auto existingZone = grid[existingX][existingY];
		if (!existingZone)
			return;

		auto localDistance = static_cast<float>(potentialPos.dist2d(int3(static_cast<si32>(existingX), static_cast<si32>(existingY), 0)));
		localDistance *= scaleForceBetweenZones(zone, existingZone);
		distance += localDistance;
	});
	return distance;
}

float CZoneGridPlacer::sumDistanceToAnchorsScaled(
	size_t x,
	size_t y,
	size_t gridSize,
	const std::vector<std::shared_ptr<Zone>> & anchors,
	const std::vector<size_t> & gridSizes,
	const std::map<TRmgTemplateZoneId, int3> & placedPositions) const
{
	float sumDist = 0;
	for (const auto & anchor : anchors)
	{
		const int3 anchorPos = placedPositions.at(anchor->getId());
		const float scale = static_cast<float>(gridSize) / static_cast<float>(gridSizes[anchorPos.z]);
		const int3 at(static_cast<si32>(x), static_cast<si32>(y), 0);
		const int3 anchorXY(anchorPos.x, anchorPos.y, 0);
		sumDist += static_cast<float>(at.dist2d(anchorXY * static_cast<double>(scale)));
	}
	return sumDist;
}

CZoneGridPlacer::PlacementDecision CZoneGridPlacer::findPlacementWithoutAnchors(
	const std::shared_ptr<Zone> & zone,
	const GridType & grid,
	size_t gridSize,
	int level,
	bool levelHasZones,
	vstd::RNG * rand) const
{
	PlacementDecision decision;
	decision.bestPos = int3(-1, -1, level);

	if (!levelHasZones)
	{
		size_t x = 0;
		size_t y = 0;

		switch (zone->getType())
		{
			case ETemplateZoneType::PLAYER_START:
			case ETemplateZoneType::CPU_START:
				if (zone->getConnectedZoneIds().size() > 2)
				{
					getRandomGridEdge(rand, gridSize, x, y);
				}
				else
				{
					getRandomGridCorner(rand, gridSize, x, y);
				}
				break;
			case ETemplateZoneType::TREASURE:
				if (gridSize & 1) // odd
				{
					x = y = gridSize / 2;
				}
				else
				{
					x = (gridSize / 2) - 1 + rand->nextInt(0, 1);
					y = (gridSize / 2) - 1 + rand->nextInt(0, 1);
				}
				break;
			case ETemplateZoneType::JUNCTION:
				getRandomGridEdge(rand, gridSize, x, y);
				break;
			default:
				break;
		}

		decision.bestPos = int3(x, y, level);
		decision.foundPos = true;
		return decision;
	}

	float maxDistance = -std::numeric_limits<float>::infinity();
	float bestTieBreak = -std::numeric_limits<float>::infinity();

	forEachInGrid(gridSize, [this, &grid, &zone, &maxDistance, &bestTieBreak, &decision, gridSize, level](size_t freeX, size_t freeY)
	{
		if (grid[freeX][freeY])
			return;

		const float distance = sumWeightedDistanceToPlacedZones(grid, gridSize, zone, freeX, freeY);
		const float tie = calculateUnconnectedTieBreak(int3(freeX, freeY, level), zone, grid, gridSize);
		if (betterByPrimaryThenTie(true, distance, maxDistance, tie, bestTieBreak))
		{
			maxDistance = distance;
			bestTieBreak = tie;
			decision.bestPos = int3(freeX, freeY, level);
			decision.foundPos = true;
			decision.score = static_cast<double>(distance);
			decision.hasScore = true;
		}
	});

	return decision;
}

CZoneGridPlacer::PlacementDecision CZoneGridPlacer::findPlacementWithAnchors(
	const std::shared_ptr<Zone> & zone,
	const std::vector<std::shared_ptr<Zone>> & anchors,
	const GridType & grid,
	const std::vector<size_t> & gridSizes,
	size_t gridSize,
	int level,
	const std::map<TRmgTemplateZoneId, int3> & placedPositions) const
{
	PlacementDecision decision;
	decision.bestPos = int3(-1, -1, level);

	std::map<std::pair<int, int>, float> candidateScores;
	constexpr float anchorOrthogonalWeight = 1.0f;
	constexpr float anchorDiagonalWeight = 0.7f;
	constexpr auto anchorNeighborDirs = int3::getDirs();
	const int3 origin2d;

	for (const auto & anchor : anchors)
	{
		int3 anchorPos = placedPositions.at(anchor->getId());

		if (anchorPos.z == level)
		{
			for (const int3 & d : anchorNeighborDirs)
			{
				const int3 cell = anchorPos + d;
				if (!isWithinGrid(cell, gridSize))
					continue;
				const ui32 dSq = d.dist2dSQ(origin2d);
				const float w = (dSq == 1) ? anchorOrthogonalWeight : anchorDiagonalWeight;
				candidateScores[{cell.x, cell.y}] += w;
			}
		}
		else
		{
			const double scale = static_cast<double>(gridSize) / static_cast<double>(gridSizes[anchorPos.z]);
			int3 cell(
				static_cast<si32>(std::round(anchorPos.x * scale)),
				static_cast<si32>(std::round(anchorPos.y * scale)),
				0);
			cell.x = std::clamp(cell.x, 0, static_cast<si32>(gridSize) - 1);
			cell.y = std::clamp(cell.y, 0, static_cast<si32>(gridSize) - 1);
			candidateScores[{cell.x, cell.y}] += anchorOrthogonalWeight;
		}
	}

	float maxScore = -std::numeric_limits<float>::infinity();
	float bestTieBreakAnchors = -std::numeric_limits<float>::infinity();
	for (const auto & [pos, score] : candidateScores)
	{
		if (grid[pos.first][pos.second])
			continue;

		const float tie = calculateUnconnectedTieBreak(int3(pos.first, pos.second, level), zone, grid, gridSize);
		if (betterByPrimaryThenTie(true, score, maxScore, tie, bestTieBreakAnchors))
		{
			maxScore = score;
			bestTieBreakAnchors = tie;
			decision.bestPos = int3(pos.first, pos.second, level);
			decision.foundPos = true;
			decision.score = static_cast<double>(score);
			decision.hasScore = true;
		}
	}

	if (decision.foundPos)
		return decision;

	float minSumDist = std::numeric_limits<float>::infinity();
	float bestTieBreakFallback = -std::numeric_limits<float>::infinity();
	forEachInGrid(gridSize, [this, &grid, &zone, &anchors, &gridSizes, &placedPositions, gridSize, level, &minSumDist, &bestTieBreakFallback, &decision](size_t x, size_t y)
	{
		if (grid[x][y])
			return;

		const float sumDist = sumDistanceToAnchorsScaled(x, y, gridSize, anchors, gridSizes, placedPositions);
		const float tie = calculateUnconnectedTieBreak(int3(x, y, level), zone, grid, gridSize);
		if (betterByPrimaryThenTie(false, sumDist, minSumDist, tie, bestTieBreakFallback))
		{
			minSumDist = sumDist;
			bestTieBreakFallback = tie;
			decision.bestPos = int3(x, y, level);
			decision.foundPos = true;
			decision.score = static_cast<double>(sumDist);
			decision.hasScore = true;
		}
	});

	return decision;
}

void CZoneGridPlacer::logPlacement(
	const std::shared_ptr<Zone> & zone,
	int level,
	size_t gridSize,
	const PlacementDecision & decision) const
{
	if (!decision.foundPos)
	{
		logGlobal->warn("Could not find place for zone %d on level %d grid size %d", zone->getId(), level, gridSize);
		return;
	}

	if (decision.hasScore)
	{
		logGlobal->trace(
			"placeOnGrid: zone %d level %d grid %s score %.6g",
			zone->getId(),
			level,
			decision.bestPos.toString(),
			decision.score);
	}
	else
	{
		logGlobal->trace(
			"placeOnGrid: zone %d level %d grid %s",
			zone->getId(),
			level,
			decision.bestPos.toString());
	}
}

void CZoneGridPlacer::logInitialGrid(
	const std::vector<std::unique_ptr<GridType>> & grids,
	const std::vector<size_t> & gridSizes,
	int mapLevels) const
{

#define ZONE_PLACEMENT_LOG
#ifdef ZONE_PLACEMENT_LOG
	logGlobal->trace("Initial zone grid:");
	for (int level = 0; level < mapLevels; ++level)
	{
		if (!grids[level])
			continue;

		const auto & grid = *grids[level];
		size_t gridSize = gridSizes[level];
		logGlobal->trace("Level %d:", level);

		for (size_t x = 0; x < gridSize; ++x)
		{
			std::string s;
			for (size_t y = 0; y < gridSize; ++y)
			{
				if (grid[x][y])
					s += (boost::format("%3d ") % grid[x][y]->getId()).str();
				else
					s += " -- ";
			}
			logGlobal->trace(s);
		}
	}
#endif
}

void CZoneGridPlacer::setInitialZoneCenters(
	const std::vector<std::unique_ptr<GridType>> & grids,
	const std::vector<size_t> & gridSizes,
	int mapLevels,
	vstd::RNG * rand) const
{
	for (int level = 0; level < mapLevels; ++level)
	{
		if (!grids[level])
			continue;

		const auto & grid = *grids[level];
		size_t gridSize = gridSizes[level];

		forEachInGrid(gridSize, [&grid, &rand, gridSize, level](size_t x, size_t y)
		{
			auto zone = grid[x][y];
			if (!zone)
				return;

			// i.e. for grid size 5 we get range (0.25 - 4.75)
			auto targetX = rand->nextDouble(x + 0.25f, x + 0.75f);
			vstd::abetween(targetX, 0.5, gridSize - 0.5);
			auto targetY = rand->nextDouble(y + 0.25f, y + 0.75f);
			vstd::abetween(targetY, 0.5, gridSize - 0.5);

			zone->setCenter(float3(targetX / gridSize, targetY / gridSize, level));
		});
	}
}

void CZoneGridPlacer::placeOnGrid(const ZoneMap & zones, vstd::RNG * rand) const
{
	assert(!zones.empty());

	const int mapLevels = map.getMapGenOptions().getLevels();
	std::vector<size_t> gridSizes;
	auto grids = createGrids(zones, mapLevels, gridSizes);
	std::vector<bool> levelHasZones(mapLevels, false);
	std::map<TRmgTemplateZoneId, int3> placedPositions;

	for (const auto & pair : zones)
	{
		auto zone = pair.second;
		const int level = zone->getCenter().z;
		auto & grid = getGridForLevel(grids, level);
		const size_t gridSize = gridSizes[level];

		const auto anchors = findAnchors(zone, zones, placedPositions);
		const PlacementDecision decision = anchors.empty()
			? findPlacementWithoutAnchors(zone, grid, gridSize, level, levelHasZones[level], rand)
			: findPlacementWithAnchors(zone, anchors, grid, gridSizes, gridSize, level, placedPositions);

		logPlacement(zone, level, gridSize, decision);

		if (!decision.foundPos)
			continue;

		grid[decision.bestPos.x][decision.bestPos.y] = zone;
		placedPositions[zone->getId()] = decision.bestPos;
		levelHasZones[level] = true;
	}

	logInitialGrid(grids, gridSizes, mapLevels);
	setInitialZoneCenters(grids, gridSizes, mapLevels, rand);
}

VCMI_LIB_NAMESPACE_END
