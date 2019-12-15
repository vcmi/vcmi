/*
* PathfindingManager.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "PathfindingManager.h"
#include "AIPathfinder.h"
#include "AIPathfinderConfig.h"
#include "../Goals/Goals.h"
#include "../../../lib/CGameInfoCallback.h"
#include "../../../lib/mapping/CMap.h"

PathfindingManager::PathfindingManager(CPlayerSpecificInfoCallback * CB, VCAI * AI)
	: ai(AI), cb(CB)
{
}

void PathfindingManager::init(CPlayerSpecificInfoCallback * CB)
{
	cb = CB;
	pathfinder.reset(new AIPathfinder(cb, ai));
	pathfinder->init();
}

void PathfindingManager::setAI(VCAI * AI)
{
	ai = AI;
}

std::vector<AIPath> PathfindingManager::getPathsToTile(const HeroPtr & hero, const int3 & tile) const
{
	auto paths = pathfinder->getPathInfo(tile);

	vstd::erase_if(paths, [&](AIPath & path) -> bool{
		return path.targetHero != hero.h;
	});

	return paths;
}

std::vector<AIPath> PathfindingManager::getPathsToTile(const int3 & tile) const
{
	return pathfinder->getPathInfo(tile);
}

void PathfindingManager::updatePaths(std::vector<HeroPtr> heroes, bool useHeroChain)
{
	logAi->debug("AIPathfinder has been reseted.");
	pathfinder->updatePaths(heroes, useHeroChain);
}
