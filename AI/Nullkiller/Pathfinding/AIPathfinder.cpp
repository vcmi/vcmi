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

std::shared_ptr<AINodeStorage> AIPathfinder::storage;

AIPathfinder::AIPathfinder(CPlayerSpecificInfoCallback * cb, VCAI * ai)
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
		return std::vector<AIPath>();
	}

	return storage->getChainInfo(tile, !tileInfo->isWater());
}

void AIPathfinder::updatePaths(std::vector<HeroPtr> heroes, bool useHeroChain)
{
	if(!storage)
	{
		storage = std::make_shared<AINodeStorage>(cb->getMapSize());
	}

	logAi->debug("Recalculate all paths");
	int pass = 0;

	storage->clear();
	storage->setHeroes(heroes, ai);

	if(useHeroChain)
	{
		storage->setTownsAndDwellings(cb->getTownsInfo(), ai->visitableObjs);
	}

	auto config = std::make_shared<AIPathfinding::AIPathfinderConfig>(cb, ai, storage);
	bool continueCalculation = false;

	do
	{
		do
		{
			logAi->trace("Recalculate paths pass %d", pass++);
			cb->calculatePaths(config);
		} while(useHeroChain && storage->calculateHeroChain());

		if(!useHeroChain)
			break;

		if(storage->calculateHeroChainFinal())
		{
			logAi->trace("Recalculate paths pass final");
			cb->calculatePaths(config);
		}

		continueCalculation = storage->increaseHeroChainTurnLimit() && storage->calculateHeroChain();
	} while(continueCalculation);
}
