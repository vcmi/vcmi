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
#include "../AIUtility.h"

namespace NKAI
{

class Nullkiller;

struct PathfinderSettings
{
	bool useHeroChain;
	uint8_t scoutTurnDistanceLimit;
	uint8_t mainTurnDistanceLimit;
	bool allowBypassObjects;

	PathfinderSettings()
		:useHeroChain(false),
		scoutTurnDistanceLimit(255),
		mainTurnDistanceLimit(255),
		allowBypassObjects(true)
	{ }
};

class AIPathfinder
{
private:
	std::shared_ptr<AINodeStorage> storage;
	CPlayerSpecificInfoCallback * cb;
	Nullkiller * ai;
	std::map<ObjectInstanceID, std::unique_ptr<GraphPaths>>  heroGraphs;

public:
	AIPathfinder(CPlayerSpecificInfoCallback * cb, Nullkiller * ai);
	void calculatePathInfo(std::vector<AIPath> & paths, const int3 & tile, bool includeGraph = false) const;
	bool isTileAccessible(const HeroPtr & hero, const int3 & tile) const;
	void updatePaths(const std::map<const CGHeroInstance *, HeroRole> & heroes, PathfinderSettings pathfinderSettings);
	void updateGraphs(const std::map<const CGHeroInstance *, HeroRole> & heroes);
	void calculateQuickPathsWithBlocker(std::vector<AIPath> & result, const std::vector<const CGHeroInstance *> & heroes, const int3 & tile);
	void init();

	std::shared_ptr<AINodeStorage>getStorage()
	{
		return storage;
	}

	std::vector<AIPath> getPathInfo(const int3 & tile, bool includeGraph = false)
	{
		std::vector<AIPath> result;

		calculatePathInfo(result, tile, includeGraph);

		return result;
	}
};

}
