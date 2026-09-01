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

CZoneGridPlacer::CZoneGridPlacer(const RmgMap & map, const DistanceMap & distancesBetweenZones, float playerRepulsion,
	const std::set<TRmgTemplateZoneId> & playerZones, const std::set<TRmgTemplateZoneId> & participatingPlayerZones)
	: map(map)
	, distancesBetweenZones(distancesBetweenZones)
	, playerRepulsion(playerRepulsion)
	, playerZones(playerZones)
	, participatingPlayerZones(participatingPlayerZones)
{
}

int CZoneGridPlacer::cellDistance(const int3 & a, const int3 & b) const
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

std::pair<double, double> CZoneGridPlacer::normalizedCellPos(const int3 & cell, size_t gridSize, double offsetX, double offsetY) const
{
	// odd-r hex packing: odd rows shifted +0.5 in x, rows spaced by sqrt(3)/2. Both axes share the x scale
	// so that neighbours stay equidistant, which leaves the shorter y axis with slack - center it,
	// otherwise the whole layout is pushed against the top edge of the map.
	const double spanY = gridSize * HEX_ROW_HEIGHT;
	const double scale = 1.0 / (gridSize + 0.5);
	const double rawX = (cell.x + offsetX) + 0.5 * (cell.y & 1);
	const double rawY = (cell.y + offsetY) * HEX_ROW_HEIGHT;
	return {rawX * scale, rawY * scale + (1.0 - spanY * scale) / 2};
}

int3 CZoneGridPlacer::nearestCell(const std::pair<double, double> & normPos, size_t gridSize) const
{
	int3 best(0, 0, 0);
	double bestDist = std::numeric_limits<double>::infinity();
	forEachInGrid(gridSize, [&](size_t x, size_t y)
	{
		const int3 cell(static_cast<si32>(x), static_cast<si32>(y), 0);
		const auto center = normalizedCellPos(cell, gridSize);
		const double d = std::hypot(center.first - normPos.first, center.second - normPos.second);
		if(d < bestDist)
		{
			bestDist = d;
			best = cell;
		}
	});
	return best;
}

void CZoneGridPlacer::annealGrids(std::vector<std::unique_ptr<GridType>> & grids, const std::vector<size_t> & gridSizes, int mapLevels, vstd::RNG * rand) const
{
	// Flat working state over all levels. A zone's level is fixed here - only its cell can change, so
	// cross-level partners are realigned by rearranging same-level assignments, never by changing level.
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

	constexpr float crossAlignWeight = 6.0f; // reward for cross-level partners sharing a normalized cell
	// Repulsion is only a tie-break: pulling one connected pair a cell apart costs 1, so the total a zone
	// can gain from all its repulsive partners must stay below that - hence the cap, divided per partner.
	constexpr float repulsionCap = 0.5f;
	constexpr float connectionRepulsion = repulsionCap / 4; // a zone rarely has more than a few repulsive connections
	// halved on top of the per-partner split, because a pair of participating players counts double below
	const float playerRepulsionWeight = playerRepulsion * repulsionCap / (2 * std::max<size_t>(2, playerZones.size()));

	// Cost of one zone's edges. Every term must stay symmetric in both endpoints, otherwise the
	// incremental delta of a move would drift away from the total.
	auto zoneCost = [&](const std::shared_ptr<Zone> & z) -> float
	{
		float cost = 0;
		const int3 zc = pos.at(z->getId());
		const auto zCenter = normalizedCellPos(zc, gridSizes[zc.z]);

		// Measured geometrically rather than in hex hops: every non-adjacent neighbour of a given cell is
		// exactly 2 hops away, so hop count cannot tell a partner on the opposite side from one 120 degrees
		// around. Normalized by one cell, so a repulsive pair sitting side by side still costs "weight".
		const double cellSpan = 1.0 / gridSizes[zc.z];
		auto repulsion = [&](const int3 & oc, float weight)
		{
			if (oc.z != zc.z)
				return 0.f;

			const auto oCenter = normalizedCellPos(oc, gridSizes[oc.z]);
			const double distance = std::hypot(zCenter.first - oCenter.first, zCenter.second - oCenter.second);
			return static_cast<float>(weight * cellSpan / std::max(distance, cellSpan));
		};

		for (const auto & conn : z->getConnections())
		{
			if (conn.getZoneA() == conn.getZoneB())
				continue;
			auto it = pos.find(conn.getOtherZoneId(z->getId()));
			if (it == pos.end())
				continue;
			const int3 oc = it->second;

			if (conn.getConnectionType() == rmg::EConnectionType::REPULSIVE)
				cost += repulsion(oc, connectionRepulsion);
			else if (!conn.needsPassage())
				continue;
			else if (oc.z == zc.z)
				cost += static_cast<float>(std::max(0, cellDistance(zc, oc) - 1));
			else
			{
				const auto oCenter = normalizedCellPos(oc, gridSizes[oc.z]);
				cost += crossAlignWeight * static_cast<float>(std::hypot(zCenter.first - oCenter.first, zCenter.second - oCenter.second));
			}
		}

		if (vstd::contains(playerZones, z->getId()))
		{
			const bool weParticipate = vstd::contains(participatingPlayerZones, z->getId());
			auto ourDistances = distancesBetweenZones.find(z->getId());
			for (const auto & otherId : playerZones)
			{
				//directly connected player zones are meant to touch, they must not be pushed apart
				const bool connected = ourDistances != distancesBetweenZones.end()
					&& ourDistances->second.count(otherId) && ourDistances->second.at(otherId) == 1;
				if (otherId == z->getId() || connected)
					continue;

				//players in the game push twice as hard, so they end up at the far ends of the spread
				const bool bothParticipate = weParticipate && vstd::contains(participatingPlayerZones, otherId);
				cost += repulsion(pos.at(otherId), playerRepulsionWeight * (bothParticipate ? 2.f : 1.f));
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
		if (!conn.needsPassage())
			continue;

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

		const int3 existingCell(static_cast<si32>(existingX), static_cast<si32>(existingY), 0);
		acc += static_cast<float>(cellDistance(gridPos, existingCell) * graphDist);
	});
	return acc;
}

float CZoneGridPlacer::sumDistanceToPlacedZones(
	const GridType & grid,
	size_t gridSize,
	size_t freeX,
	size_t freeY) const
{
	const int3 potentialPos(static_cast<si32>(freeX), static_cast<si32>(freeY), 0);
	float distance = 0;
	forEachInGrid(gridSize, [this, &grid, &potentialPos, &distance](size_t existingX, size_t existingY)
	{
		if (!grid[existingX][existingY])
			return;

		const int3 existingCell(static_cast<si32>(existingX), static_cast<si32>(existingY), 0);
		distance += static_cast<float>(cellDistance(potentialPos, existingCell));
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
	// Anchors may sit on another level, whose grid has a different size - compare through normalized
	// positions, the same way annealGrids measures cross-level alignment.
	const auto at = normalizedCellPos(int3(static_cast<si32>(x), static_cast<si32>(y), 0), gridSize);
	float sumDist = 0;
	for (const auto & anchor : anchors)
	{
		const int3 anchorPos = placedPositions.at(anchor->getId());
		const auto anchorAt = normalizedCellPos(anchorPos, gridSizes[anchorPos.z]);
		sumDist += static_cast<float>(std::hypot(at.first - anchorAt.first, at.second - anchorAt.second));
	}
	return sumDist;
}

CZoneGridPlacer::PlacementDecision CZoneGridPlacer::findPlacementWithoutAnchors(
	const std::shared_ptr<Zone> & zone,
	const GridType & grid,
	size_t gridSize,
	int level,
	bool levelHasZones) const
{
	PlacementDecision decision;
	decision.bestPos = int3(-1, -1, level);

	if (!levelHasZones)
	{
		// first zone on the level is the highest-degree hub - seed it centrally where the most
		// neighbours are reachable, and grow the rest outward around it
		decision.bestPos = int3(gridSize / 2, gridSize / 2, level);
		decision.foundPos = true;
		return decision;
	}

	float maxDistance = -std::numeric_limits<float>::infinity();
	float bestTieBreak = -std::numeric_limits<float>::infinity();

	forEachInGrid(gridSize, [this, &grid, &zone, &maxDistance, &bestTieBreak, &decision, gridSize, level](size_t freeX, size_t freeY)
	{
		if (grid[freeX][freeY])
			return;

		const float distance = sumDistanceToPlacedZones(grid, gridSize, freeX, freeY);
		const float tie = calculateUnconnectedTieBreak(int3(freeX, freeY, level), zone, grid, gridSize);
		if (betterByPrimaryThenTie(true, distance, maxDistance, tie, bestTieBreak))
		{
			maxDistance = distance;
			bestTieBreak = tie;
			decision.bestPos = int3(freeX, freeY, level);
			decision.foundPos = true;
			decision.score = static_cast<double>(distance);
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
	constexpr float anchorNeighbourWeight = 1.0f;

	for (const auto & anchor : anchors)
	{
		int3 anchorPos = placedPositions.at(anchor->getId());

		if (anchorPos.z == level)
		{
			for (const int3 & d : hexNeighbourDirs(anchorPos.y))
			{
				const int3 cell = anchorPos + d;
				if (isWithinGrid(cell, gridSize))
					candidateScores[{cell.x, cell.y}] += anchorNeighbourWeight;
			}
		}
		else
		{
			// cross-level partner: aim for the cell overlapping it, so a subterranean gate is possible
			const int3 cell = nearestCell(normalizedCellPos(anchorPos, gridSizes[anchorPos.z]), gridSize);
			candidateScores[{cell.x, cell.y}] += anchorNeighbourWeight;
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

	logGlobal->trace(
		"placeOnGrid: zone %d level %d grid %s score %.6g",
		zone->getId(),
		level,
		decision.bestPos.toString(),
		decision.score);
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

		forEachInGrid(gridSize, [this, &grid, &rand, gridSize, level](size_t x, size_t y)
		{
			auto zone = grid[x][y];
			if (!zone)
				return;

			const int3 cell(static_cast<si32>(x), static_cast<si32>(y), level);
			const auto [targetX, targetY] = normalizedCellPos(cell, gridSize, rand->nextDouble(0.25, 0.75), rand->nextDouble(0.25, 0.75));
			zone->setCenter(float3(static_cast<float>(targetX), static_cast<float>(targetY), level));
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

	// Highest-degree zones go first, so each level's hub lands centrally and every later zone finds its
	// anchors already placed. Shuffle first so equal degrees are broken by seed rather than by zone id.
	std::vector<std::shared_ptr<Zone>> order;
	order.reserve(zones.size());
	for (const auto & pair : zones)
		order.push_back(pair.second);
	RandomGeneratorUtil::randomShuffle(order, *rand);
	//degree = number of real (non-virtual, non-self) connections
	auto zoneDegree = [](const std::shared_ptr<Zone> & zone)
	{
		return std::ranges::count_if(zone->getConnections(), &rmg::ZoneConnection::needsPassage);
	};
	std::ranges::stable_sort(order, [&zoneDegree](const std::shared_ptr<Zone> & a, const std::shared_ptr<Zone> & b)
	{
		return zoneDegree(a) > zoneDegree(b);
	});

	for (const auto & zone : order)
	{
		const int level = zone->getCenter().z;
		auto & grid = *grids.at(level);
		const size_t gridSize = gridSizes[level];

		const auto anchors = findAnchors(zone, zones, placedPositions);
		const PlacementDecision decision = anchors.empty()
			? findPlacementWithoutAnchors(zone, grid, gridSize, level, levelHasZones[level])
			: findPlacementWithAnchors(zone, anchors, grid, gridSizes, gridSize, level, placedPositions);

		logPlacement(zone, level, gridSize, decision);

		if (!decision.foundPos)
			continue;

		grid[decision.bestPos.x][decision.bestPos.y] = zone;
		placedPositions[zone->getId()] = decision.bestPos;
		levelHasZones[level] = true;
	}

	annealGrids(grids, gridSizes, mapLevels, rand);
	setInitialZoneCenters(grids, gridSizes, mapLevels, rand);
}
