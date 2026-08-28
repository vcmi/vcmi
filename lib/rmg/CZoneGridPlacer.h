#pragma once

#include "../GameConstants.h"
#include "../int3.h"

#include <boost/multi_array.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace vstd
{
class RNG;
}

class RmgMap;
class Zone;

class CZoneGridPlacer
{
public:
	using ZoneMap = std::map<TRmgTemplateZoneId, std::shared_ptr<Zone>>;
	using DistanceMap = std::map<int, std::map<int, size_t>>;

	CZoneGridPlacer(const RmgMap & map, const DistanceMap & distancesBetweenZones, float playerRepulsion,
		const std::set<TRmgTemplateZoneId> & playerZones, const std::set<TRmgTemplateZoneId> & participatingPlayerZones);

	void placeOnGrid(const ZoneMap & zones, vstd::RNG * rand) const;

private:
	using GridType = boost::multi_array<std::shared_ptr<Zone>, 2>;

	struct PlacementDecision
	{
		int3 bestPos;
		bool foundPos = false;
		double score = 0.0; // best value of whichever criterion the search used; 0 for an unconstrained seed
	};

	/// True if cell x,y are in [0, gridSize); z is ignored.
	static bool isWithinGrid(const int3 & cell, size_t gridSize);
	static bool betterByPrimaryThenTie(
		bool maximizePrimary,
		float candidatePrimary,
		float bestPrimary,
		float candidateTie,
		float bestTie);

	std::vector<std::unique_ptr<GridType>> createGrids(const ZoneMap & zones, int mapLevels, std::vector<size_t> & gridSizes) const;
	std::vector<std::shared_ptr<Zone>> findAnchors(
		const std::shared_ptr<Zone> & zone,
		const ZoneMap & zones,
		const std::map<TRmgTemplateZoneId, int3> & placedPositions) const;
	float calculateUnconnectedTieBreak(
		const int3 & gridPos,
		const std::shared_ptr<Zone> & zone,
		const GridType & grid,
		size_t gridSize) const;
	float sumDistanceToPlacedZones(
		const GridType & grid,
		size_t gridSize,
		size_t freeX,
		size_t freeY) const;
	float sumDistanceToAnchorsScaled(
		size_t x,
		size_t y,
		size_t gridSize,
		const std::vector<std::shared_ptr<Zone>> & anchors,
		const std::vector<size_t> & gridSizes,
		const std::map<TRmgTemplateZoneId, int3> & placedPositions) const;
	PlacementDecision findPlacementWithoutAnchors(
		const std::shared_ptr<Zone> & zone,
		const GridType & grid,
		size_t gridSize,
		int level,
		bool levelHasZones) const;
	PlacementDecision findPlacementWithAnchors(
		const std::shared_ptr<Zone> & zone,
		const std::vector<std::shared_ptr<Zone>> & anchors,
		const GridType & grid,
		const std::vector<size_t> & gridSizes,
		size_t gridSize,
		int level,
		const std::map<TRmgTemplateZoneId, int3> & placedPositions) const;
	void logPlacement(
		const std::shared_ptr<Zone> & zone,
		int level,
		size_t gridSize,
		const PlacementDecision & decision) const;
	void setInitialZoneCenters(
		const std::vector<std::unique_ptr<GridType>> & grids,
		const std::vector<size_t> & gridSizes,
		int mapLevels,
		vstd::RNG * rand) const;

	/// Hex (cube) distance between two cells. Connected zones are "adjacent" when this equals 1.
	int cellDistance(const int3 & a, const int3 & b) const;
	/// Normalized (0..1) position of a point in a grid cell; offsets are fractions of the cell (0.5 = center).
	std::pair<double, double> normalizedCellPos(const int3 & cell, size_t gridSize, double offsetX = 0.5, double offsetY = 0.5) const;
	/// Cell of this grid whose center is closest to a normalized position. Levels have differently sized
	/// grids, so comparing cells across levels only makes sense through normalized positions.
	int3 nearestCell(const std::pair<double, double> & normPos, size_t gridSize) const;
	/// Simulated-annealing polish of the placement, in place and across all levels jointly: swaps
	/// zone<->cell assignments within each level to make connected zones adjacent (or, across levels,
	/// aligned) while keeping repulsive pairs apart.
	void annealGrids(std::vector<std::unique_ptr<GridType>> & grids, const std::vector<size_t> & gridSizes, int mapLevels, vstd::RNG * rand) const;

	const RmgMap & map;
	const DistanceMap & distancesBetweenZones;
	float playerRepulsion; // strength of the preference for keeping player starts apart (0 = off)
	const std::set<TRmgTemplateZoneId> & playerZones; // every player starting zone
	const std::set<TRmgTemplateZoneId> & participatingPlayerZones; // those belonging to players in the game
};
