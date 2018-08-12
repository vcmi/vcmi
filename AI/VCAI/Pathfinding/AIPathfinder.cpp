/*
* AIhelper.h, part of VCMI engine
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

std::vector<std::shared_ptr<AINodeStorage>> AIPathfinder::storagePool;
std::map<HeroPtr, std::shared_ptr<AINodeStorage>> AIPathfinder::storageMap;
boost::mutex AIPathfinder::storageMutex;

AIPathfinder::AIPathfinder(CPlayerSpecificInfoCallback * cb)
	:cb(cb)
{
}

void AIPathfinder::clear()
{
	boost::unique_lock<boost::mutex> storageLock(storageMutex);
	storageMap.clear();
}

std::vector<AIPath> AIPathfinder::getPathInfo(HeroPtr hero, int3 tile)
{
	boost::unique_lock<boost::mutex> storageLock(storageMutex);
	std::shared_ptr<AINodeStorage> nodeStorage;

	if(!vstd::contains(storageMap, hero))
	{
		logAi->debug("Recalculate paths for %s", hero->name);

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
		
		auto config = std::make_shared<AIPathfinderConfig>(cb, nodeStorage);

		nodeStorage->setHero(hero.get());
		cb->calculatePaths(config, hero.get());
	}
	else
	{
		nodeStorage = storageMap.at(hero);
	}

	return nodeStorage->getChainInfo(tile);
}