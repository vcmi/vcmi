/*
* AIPathfinder.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "AINodeStorage.h"
#include "ObjectGraph.h"
#include "GraphPaths.h"
#include "../AIUtility.h"

namespace NK2AI
{

class Nullkiller;

struct PathfinderSettings
{
	static constexpr uint8_t MaxTurnDistanceLimit = 255;

	bool useHeroChain;
	uint8_t scoutTurnDistanceLimit;
	uint8_t mainTurnDistanceLimit;
	bool allowBypassObjects;

	PathfinderSettings()
		:useHeroChain(false),
		scoutTurnDistanceLimit(MaxTurnDistanceLimit),
		mainTurnDistanceLimit(MaxTurnDistanceLimit),
		allowBypassObjects(true)
	{ }
};

class AIPathfinder
{
private:
	std::shared_ptr<AINodeStorage> storage;
	Nullkiller * aiNk;
	static std::map<ObjectInstanceID, std::unique_ptr<GraphPaths>>  heroGraphs;

public:
	explicit AIPathfinder(Nullkiller * aiNk);
	void calculatePathInfo(std::vector<AIPath> & paths, const int3 & tile, bool includeGraph = false) const;
	bool isTileAccessible(const HeroPtr & hero, const int3 & tile) const;
	void updatePaths(const std::map<const CGHeroInstance *, HeroRole> & heroes, PathfinderSettings pathfinderSettings);
	void updateGraphs(const std::map<const CGHeroInstance *, HeroRole> & heroes, uint8_t mainScanDepth, uint8_t scoutScanDepth);
	void calculateQuickPathsWithBlocker(std::vector<AIPath> & result, const std::vector<const CGHeroInstance *> & heroes, const int3 & tile);

	std::shared_ptr<AINodeStorage>getStorage()
	{
		return storage;
	}

	std::vector<AIPath> getPathInfo(const int3 & tile, const bool includeGraph = false) const
	{
		std::vector<AIPath> result;
		calculatePathInfo(result, tile, includeGraph);
		return result;
	}
};

}
