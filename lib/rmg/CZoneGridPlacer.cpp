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

namespace
{
template <typename Fn>
void forEachInGrid(const size_t gridSize, Fn && fn)
{
	for (size_t x = 0; x < gridSize; ++x)
		for (size_t y = 0; y < gridSize; ++y)
			fn(x, y);
}

constexpr double HEX_ROW_HEIGHT = 0.86602540378443864; // sqrt(3)/2 - vertical spacing between hex rows

// odd-r offset hex neighbours: odd rows are shifted +0.5 in x, so the 6 neighbour offsets depend on row parity
std::array<int3, 6> hexNeighbourDirs(int row)
{
	if (row & 1)
		return {{ {+1, 0, 0}, {+1, -1, 0}, {0, -1, 0}, {-1, 0, 0}, {0, +1, 0}, {+1, +1, 0} }};
	return {{ {+1, 0, 0}, {0, -1, 0}, {-1, -1, 0}, {-1, 0, 0}, {-1, +1, 0}, {0, +1, 0} }};
}
}

CZoneGridPlacer::CZoneGridPlacer(const RmgMap & map, const DistanceMap & distancesBetweenZones, ScaleForceFn scaleForceBetweenZones, bool hexGrid, bool hubFirst, bool saPolish, float crossAlignWeight)
	: map(map)
	, distancesBetweenZones(distancesBetweenZones)
	, scaleForceBetweenZones(std::move(scaleForceBetweenZones))
	, hexGrid(hexGrid)
	, hubFirst(hubFirst)
	, saPolish(saPolish)
	, crossAlignWeight(crossAlignWeight)
{
}

int CZoneGridPlacer::cellDistance(const int3 & a, const int3 & b) const
{
	if (hexGrid)
	{
		// odd-r offset -> cube coordinates, then cube distance
		auto toCube = [](const int3 & c)
		{
			const int x = c.x - (c.y - (c.y & 1)) / 2;
			const int z = c.y;
			return int3(x, -x - z, z);
		};
		const int3 ca = toCube(a);
		const int3 cb = toCube(b);
		return (std::abs(ca.x - cb.x) + std::abs(ca.y - cb.y) + std::abs(ca.z - cb.z)) / 2;
	}
	// square grid: orthogonal adjacency, so Manhattan distance
	return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

std::pair<double, double> CZoneGridPlacer::normalizedCellCenter(const int3 & cell, size_t gridSize) const
{
	if (hexGrid)
	{
		const double rawX = (cell.x + 0.5) + 0.5 * (cell.y & 1);
		const double rawY = (cell.y + 0.5) * HEX_ROW_HEIGHT;
		const double denom = gridSize + 1.0;
		return {rawX / denom, rawY / denom};
	}
	return {(cell.x + 0.5) / gridSize, (cell.y + 0.5) / gridSize};
}

void CZoneGridPlacer::annealGrids(std::vector<std::unique_ptr<GridType>> & grids, const std::vector<size_t> & gridSizes, int mapLevels, vstd::RNG * rand) const
{
	// Gather the current placement across all levels into a flat working state. Every zone's level is
	// fixed (a surface zone stays on the surface); only its cell within that level's grid can change.
	std::vector<std::shared_ptr<Zone>> zonesAll;
	std::vector<std::vector<std::shared_ptr<Zone>>> zonesByLevel(mapLevels);
	std::vector<std::vector<int3>> emptyByLevel(mapLevels);
	std::map<TRmgTemplateZoneId, int3> pos;
	for (int level = 0; level < mapLevels; ++level)
	{
		if (!grids[level])
			continue;
		const size_t gridSize = gridSizes[level];
		auto & grid = *grids[level];
		forEachInGrid(gridSize, [&](size_t x, size_t y)
		{
			const int3 cell(static_cast<si32>(x), static_cast<si32>(y), level);
			if (grid[x][y])
			{
				zonesAll.push_back(grid[x][y]);
				zonesByLevel[level].push_back(grid[x][y]);
				pos[grid[x][y]->getId()] = cell;
			}
			else
				emptyByLevel[level].push_back(cell);
		});
	}
	if (zonesAll.size() < 3)
		return;

	// Cost of one zone's edges. Same-level edges: 0 when the connected zone is adjacent, else grows with
	// grid distance. Cross-level edges: crossAlignWeight times the normalized-center distance to the
	// partner, symmetric in both endpoints so the incremental delta stays consistent with the total.
	auto zoneCost = [&](const std::shared_ptr<Zone> & z) -> float
	{
		float cost = 0;
		const int3 zc = pos.at(z->getId());
		const auto zCenter = normalizedCellCenter(zc, gridSizes[zc.z]);
		for (const auto & conn : z->getConnections())
		{
			if (conn.getConnectionType() == rmg::EConnectionType::REPULSIVE ||
				conn.getConnectionType() == rmg::EConnectionType::FORCE_PORTAL)
				continue;
			if (conn.getZoneA() == conn.getZoneB())
				continue;
			auto it = pos.find(conn.getOtherZoneId(z->getId()));
			if (it == pos.end())
				continue;
			const int3 oc = it->second;
			if (oc.z == zc.z)
				cost += static_cast<float>(std::max(0, cellDistance(zc, oc) - 1));
			else if (crossAlignWeight > 0)
			{
				const auto oCenter = normalizedCellCenter(oc, gridSizes[oc.z]);
				cost += crossAlignWeight * static_cast<float>(std::hypot(zCenter.first - oCenter.first, zCenter.second - oCenter.second));
			}
		}
		return cost;
	};

	float current = 0;
	for (const auto & z : zonesAll)
		current += zoneCost(z);
	current *= 0.5f; // each edge counted from both endpoints

	auto best = pos;
	float bestCost = current;

	const int iterations = std::max<int>(2000, 300 * static_cast<int>(zonesAll.size()));
	float temperature = 3.0f;
	const float cooling = std::pow(0.02f / temperature, 1.0f / iterations); // anneal down to ~0.02

	for (int i = 0; i < iterations; ++i, temperature *= cooling)
	{
		// Pick a zone; every move keeps it on its own level, so cross-level partners are only ever
		// realigned by rearranging same-level assignments - never by pulling a zone to another level.
		const auto & z = zonesAll[rand->nextInt(0, static_cast<int>(zonesAll.size()) - 1)];
		const int level = pos[z->getId()].z;
		auto & empties = emptyByLevel[level];

		if (!empties.empty() && rand->nextInt(0, 4) == 0)
		{
			// move a zone into an empty cell on its level
			const int ei = rand->nextInt(0, static_cast<int>(empties.size()) - 1);
			const int3 oldCell = pos[z->getId()];
			const float before = zoneCost(z);
			pos[z->getId()] = empties[ei];
			const float delta = zoneCost(z) - before;
			if (delta <= 0 || rand->nextDouble(0, 1) < std::exp(-delta / temperature))
			{
				empties[ei] = oldCell;
				current += delta;
			}
			else
				pos[z->getId()] = oldCell;
		}
		else
		{
			// swap two zones' cells on the same level
			const auto & peers = zonesByLevel[level];
			if (peers.size() < 2)
				continue;
			const auto & za = z;
			const auto & zb = peers[rand->nextInt(0, static_cast<int>(peers.size()) - 1)];
			if (za == zb)
				continue;
			const int3 ca = pos[za->getId()];
			const int3 cb = pos[zb->getId()];
			const float before = zoneCost(za) + zoneCost(zb);
			pos[za->getId()] = cb;
			pos[zb->getId()] = ca;
			const float delta = (zoneCost(za) + zoneCost(zb)) - before;
			if (delta <= 0 || rand->nextDouble(0, 1) < std::exp(-delta / temperature))
				current += delta;
			else
			{
				pos[za->getId()] = ca;
				pos[zb->getId()] = cb;
			}
		}

		if (current < bestCost)
		{
			bestCost = current;
			best = pos;
		}
	}

	// commit the best assignment found, per level
	for (int level = 0; level < mapLevels; ++level)
	{
		if (!grids[level])
			continue;
		auto & grid = *grids[level];
		forEachInGrid(gridSizes[level], [&](size_t x, size_t y) { grid[x][y].reset(); });
	}
	for (const auto & z : zonesAll)
	{
		const int3 cell = best.at(z->getId());
		(*grids[cell.z])[cell.x][cell.y] = z;
	}
}

namespace
{
// Number of real (non-virtual, non-self) connections - a zone's degree in the connection graph.
int zoneDegree(const std::shared_ptr<Zone> & zone)
{
	int degree = 0;
	for (const auto & conn : zone->getConnections())
	{
		if (conn.getConnectionType() == rmg::EConnectionType::REPULSIVE ||
			conn.getConnectionType() == rmg::EConnectionType::FORCE_PORTAL)
			continue;
		if (conn.getZoneA() == conn.getZoneB())
			continue;
		degree++;
	}
	return degree;
}
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

		if (hubFirst)
		{
			// first zone on the level is the highest-degree hub - seed it centrally where the most
			// neighbours are reachable, and grow the rest outward around it
			x = gridSize / 2;
			y = gridSize / 2;
			decision.bestPos = int3(x, y, level);
			decision.foundPos = true;
			return decision;
		}

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
			if (hexGrid)
			{
				// every hex neighbour is a true edge neighbour - equal weight, no diagonal contact
				for (const int3 & d : hexNeighbourDirs(anchorPos.y))
				{
					const int3 cell = anchorPos + d;
					if (isWithinGrid(cell, gridSize))
						candidateScores[{cell.x, cell.y}] += anchorOrthogonalWeight;
				}
			}
			else
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
		const bool hex = hexGrid;

		forEachInGrid(gridSize, [&grid, &rand, gridSize, level, hex](size_t x, size_t y)
		{
			auto zone = grid[x][y];
			if (!zone)
				return;

			if (hex)
			{
				// odd-r hex packing: odd rows shifted +0.5 in x, rows spaced by sqrt(3)/2.
				// Common denominator for x and y preserves the hexagonal aspect (equidistant neighbours).
				const double rawX = (x + rand->nextDouble(0.25, 0.75)) + 0.5 * (y & 1);
				const double rawY = (y + rand->nextDouble(0.25, 0.75)) * HEX_ROW_HEIGHT;
				const double denom = gridSize + 1.0;
				zone->setCenter(float3(static_cast<float>(rawX / denom), static_cast<float>(rawY / denom), level));
				return;
			}

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

	// Build the processing order. hubFirst places highest-degree zones first, so each level's hub
	// lands (centred) before its neighbours and every later zone finds its higher-degree anchors
	// already placed. Otherwise keep the original id order.
	std::vector<std::shared_ptr<Zone>> order;
	order.reserve(zones.size());
	for (const auto & pair : zones)
		order.push_back(pair.second);
	if (hubFirst)
	{
		// Break equal-degree ties randomly (rather than by id) so the hub - and the whole construction
		// order - varies between placement attempts. The attempts loop keeps the best-scoring layout, so
		// a template with several equally-connected candidates (e.g. Grond's five degree-4 zones) settles
		// on a merit-chosen centre that differs by seed, instead of always the lowest-id zone.
		std::map<TRmgTemplateZoneId, int> tieBreak;
		for (const auto & zone : order)
			tieBreak[zone->getId()] = rand->nextInt(0, std::numeric_limits<int>::max() - 1);
		std::ranges::stable_sort(order, [&tieBreak](const std::shared_ptr<Zone> & a, const std::shared_ptr<Zone> & b)
		{
			const int da = zoneDegree(a);
			const int db = zoneDegree(b);
			if (da != db)
				return da > db;
			return tieBreak.at(a->getId()) < tieBreak.at(b->getId());
		});
	}

	for (const auto & zone : order)
	{
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
	if (saPolish)
		annealGrids(grids, gridSizes, mapLevels, rand);
	setInitialZoneCenters(grids, gridSizes, mapLevels, rand);
}
