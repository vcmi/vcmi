#pragma once

#include "../GameConstants.h"
#include "../int3.h"

#include <boost/multi_array.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

VCMI_LIB_NAMESPACE_BEGIN

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
	using ScaleForceFn = std::function<float(const std::shared_ptr<Zone> &, const std::shared_ptr<Zone> &)>;

	CZoneGridPlacer(const RmgMap & map, const DistanceMap & distancesBetweenZones, ScaleForceFn scaleForceBetweenZones);

	void placeOnGrid(const ZoneMap & zones, vstd::RNG * rand) const;

private:
	using GridType = boost::multi_array<std::shared_ptr<Zone>, 2>;

	struct PlacementDecision
	{
		int3 bestPos;
		bool foundPos = false;
		double score = 0.0;
		bool hasScore = false;
	};

	GridType & getGridForLevel(std::vector<std::unique_ptr<GridType>> & grids, int level) const;
	static void getRandomGridCorner(vstd::RNG * rand, size_t gridSize, size_t & x, size_t & y);
	static void getRandomGridEdge(vstd::RNG * rand, size_t gridSize, size_t & x, size_t & y);
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
	float sumWeightedDistanceToPlacedZones(
		const GridType & grid,
		size_t gridSize,
		const std::shared_ptr<Zone> & zone,
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
		bool levelHasZones,
		vstd::RNG * rand) const;
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
	void logInitialGrid(
		const std::vector<std::unique_ptr<GridType>> & grids,
		const std::vector<size_t> & gridSizes,
		int mapLevels) const;
	void setInitialZoneCenters(
		const std::vector<std::unique_ptr<GridType>> & grids,
		const std::vector<size_t> & gridSizes,
		int mapLevels,
		vstd::RNG * rand) const;

	const RmgMap & map;
	const DistanceMap & distancesBetweenZones;
	ScaleForceFn scaleForceBetweenZones;
};

VCMI_LIB_NAMESPACE_END
