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
#include "../../../lib/mapping/CMap.h"
#include "../Engine/Nullkiller.h"

namespace NKAI
{

AIPathfinder::AIPathfinder(CPlayerSpecificInfoCallback * cb, Nullkiller * ai)
	:cb(cb), ai(ai)
{
}

void AIPathfinder::init()
{
	storage.reset();
}

bool AIPathfinder::isTileAccessible(const HeroPtr & hero, const int3 & tile) const
{
	return storage->isTileAccessible(hero, tile, EPathfindingLayer::LAND)
		|| storage->isTileAccessible(hero, tile, EPathfindingLayer::SAIL);
}

std::vector<AIPath> AIPathfinder::getPathInfo(const int3 & tile) const
{
	const TerrainTile * tileInfo = cb->getTile(tile, false);

	if(!tileInfo)
	{
		return {};
	}

	return storage->getChainInfo(tile, !tileInfo->isWater());
}

void AIPathfinder::updatePaths(std::map<const CGHeroInstance *, HeroRole> heroes, PathfinderSettings pathfinderSettings)
{
	if(!storage)
	{
		storage.reset(new AINodeStorage(ai, cb->getMapSize()));
	}

	auto start = std::chrono::high_resolution_clock::now();
	logAi->debug("Recalculate all paths");
	int pass = 0;

	storage->clear();
	storage->setHeroes(heroes);
	storage->setScoutTurnDistanceLimit(pathfinderSettings.scoutTurnDistanceLimit);
	storage->setMainTurnDistanceLimit(pathfinderSettings.mainTurnDistanceLimit);

	if(pathfinderSettings.useHeroChain)
	{
		storage->setTownsAndDwellings(cb->getTownsInfo(), ai->memory->visitableObjs);
	}

	auto config = std::make_shared<AIPathfinding::AIPathfinderConfig>(cb, ai, storage);

	logAi->trace("Recalculate paths pass %d", pass++);
	cb->calculatePaths(config);

	if(!pathfinderSettings.useHeroChain)
		return;

	do
	{
		storage->selectFirstActor();

		do
		{
			boost::this_thread::interruption_point();

			while(storage->calculateHeroChain())
			{
				boost::this_thread::interruption_point();

				logAi->trace("Recalculate paths pass %d", pass++);
				cb->calculatePaths(config);
			}

			logAi->trace("Select next actor");
		} while(storage->selectNextActor());

		boost::this_thread::interruption_point();

		if(storage->calculateHeroChainFinal())
		{
			boost::this_thread::interruption_point();

			logAi->trace("Recalculate paths pass final");
			cb->calculatePaths(config);
		}
	} while(storage->increaseHeroChainTurnLimit());

	logAi->trace("Recalculated paths in %ld", timeElapsed(start));
}

}
