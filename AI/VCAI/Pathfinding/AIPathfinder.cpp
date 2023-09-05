/*
* AIPathfinder.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AIPathfinder.h"
#include "AIPathfinderConfig.h"
#include "../../../CCallback.h"
#include "../../../lib/mapping/CMapDefines.h"
#include "../../../lib/ThreadPool.h"

std::vector<std::shared_ptr<AINodeStorage>> AIPathfinder::storagePool;
std::map<HeroPtr, std::shared_ptr<AINodeStorage>> AIPathfinder::storageMap;

AIPathfinder::AIPathfinder(CPlayerSpecificInfoCallback * cb, VCAI * ai)
	:cb(cb), ai(ai)
{
}

void AIPathfinder::init()
{
	storagePool.clear();
	storageMap.clear();
}

bool AIPathfinder::isTileAccessible(const HeroPtr & hero, const int3 & tile) const
{
	std::shared_ptr<const AINodeStorage> nodeStorage = getStorage(hero);

	return nodeStorage->isTileAccessible(tile, EPathfindingLayer::LAND)
		|| nodeStorage->isTileAccessible(tile, EPathfindingLayer::SAIL);
}

std::vector<AIPath> AIPathfinder::getPathInfo(const HeroPtr & hero, const int3 & tile) const
{
	std::shared_ptr<const AINodeStorage> nodeStorage = getStorage(hero);

	const TerrainTile * tileInfo = cb->getTile(tile, false);

	if(!tileInfo)
	{
		return std::vector<AIPath>();
	}

	return nodeStorage->getChainInfo(tile, !tileInfo->isWater());
}

void AIPathfinder::updatePaths(std::vector<HeroPtr> heroes)
{
	storageMap.clear();

	auto calculatePaths = [&](const CGHeroInstance * hero, std::shared_ptr<AIPathfinding::AIPathfinderConfig> config)
	{
		logAi->debug("Recalculate paths for %s", hero->getNameTranslated());
		
		cb->calculatePaths(config);
	};

	ThreadPool pool(heroes.size());

	std::vector<boost::future<void>> calculationTasks;

	for(HeroPtr hero : heroes)
	{
		std::shared_ptr<AINodeStorage> nodeStorage;

		if(storageMap.size() < storagePool.size())
		{
			nodeStorage = storagePool.at(storageMap.size());
		}
		else
		{
			nodeStorage = std::make_shared<AINodeStorage>(cb->getMapSize());
			storagePool.push_back(nodeStorage);
		}

		storageMap[hero] = nodeStorage;
		nodeStorage->setHero(hero, ai);

		auto config = std::make_shared<AIPathfinding::AIPathfinderConfig>(cb, ai, nodeStorage);

		calculationTasks.push_back(pool.async(std::bind(calculatePaths, hero.get(), config)));
	}

	for (auto& fut : calculationTasks)
		fut.get();
}

std::shared_ptr<const AINodeStorage> AIPathfinder::getStorage(const HeroPtr & hero) const
{
	return storageMap.at(hero);
}

