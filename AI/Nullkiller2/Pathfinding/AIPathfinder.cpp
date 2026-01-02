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

#include "../../../lib/mapping/CMap.h"
#include "../Engine/Nullkiller.h"
#include "../AIGateway.h"
#include "AIPathfinderConfig.h"

namespace NK2AI
{

std::map<ObjectInstanceID, std::unique_ptr<GraphPaths>>  AIPathfinder::heroGraphs;

AIPathfinder::AIPathfinder(Nullkiller * aiNk) : aiNk(aiNk) {}

bool AIPathfinder::isTileAccessible(const HeroPtr & hero, const int3 & tile) const
{
	return storage->isTileAccessible(hero, tile, EPathfindingLayer::LAND)
		|| storage->isTileAccessible(hero, tile, EPathfindingLayer::SAIL);
}

void AIPathfinder::calculateQuickPathsWithBlocker(std::vector<AIPath> & result, const std::vector<const CGHeroInstance *> & heroes, const int3 & tile)
{
	result.clear();

	for(auto hero : heroes)
	{
		auto graph = heroGraphs.find(hero->id);

		if(graph != heroGraphs.end())
			graph->second->quickAddChainInfoWithBlocker(result, tile, hero, aiNk);
	}
}

void AIPathfinder::calculatePathInfo(std::vector<AIPath> & paths, const int3 & tile, bool includeGraph) const
{
	const TerrainTile * tileInfo = aiNk->cc->getTile(tile, false);
	paths.clear();
	if(!tileInfo)
		return;

	storage->calculateChainInfo(paths, tile, !tileInfo->isWater());

	if(includeGraph)
	{
		for(const auto * hero : aiNk->cc->getHeroesInfo())
		{
			auto graph = heroGraphs.find(hero->id);

			if(graph != heroGraphs.end())
				graph->second->addChainInfo(paths, tile, hero, aiNk);
		}
	}
}

void AIPathfinder::updatePaths(const std::map<const CGHeroInstance *, HeroRole> & heroes, PathfinderSettings pathfinderSettings)
{
	if(!storage)
	{
		storage.reset(new AINodeStorage(aiNk, aiNk->cc->getMapSize()));
	}

	auto start = std::chrono::high_resolution_clock::now();
	logAi->debug("Recalculate all paths");
	int pass = 0;

	storage->clear();
	storage->setHeroes(heroes);
	storage->setScoutTurnDistanceLimit(pathfinderSettings.scoutTurnDistanceLimit);
	storage->setMainTurnDistanceLimit(pathfinderSettings.mainTurnDistanceLimit);

	logAi->trace(
		"Scout turn distance: %s, main %s",
		std::to_string(pathfinderSettings.scoutTurnDistanceLimit),
		std::to_string(pathfinderSettings.mainTurnDistanceLimit));

	if(pathfinderSettings.useHeroChain)
	{
		storage->setTownsAndDwellings(aiNk->cc->getTownsInfo(), aiNk->memory->visitableIdsToObjsSet(*aiNk->cc));
	}

	const auto config = std::make_shared<AIPathfinding::AIPathfinderConfig>(aiNk, storage, pathfinderSettings.allowBypassObjects);
	logAi->trace("Recalculate paths pass %d", pass++);
	aiNk->cc->calculatePaths(config);

	if(!pathfinderSettings.useHeroChain)
	{
		logAi->trace("Recalculated paths in %ld ms", timeElapsed(start));
		return;
	}

	do
	{
		storage->selectFirstActor();

		do
		{
			aiNk->makingTurnInterruption.interruptionPoint();

			while(storage->calculateHeroChain())
			{
				aiNk->makingTurnInterruption.interruptionPoint();

				logAi->trace("Recalculate paths pass %d", pass++);
				aiNk->cc->calculatePaths(config);
			}

			logAi->trace("Select next actor");
		} while(storage->selectNextActor());

		aiNk->makingTurnInterruption.interruptionPoint();

		if(storage->calculateHeroChainFinal())
		{
			aiNk->makingTurnInterruption.interruptionPoint();

			logAi->trace("Recalculate paths pass final");
			aiNk->cc->calculatePaths(config);
		}
	} while(storage->increaseHeroChainTurnLimit());

	logAi->trace("Recalculated paths in %ld ms", timeElapsed(start));
}

void AIPathfinder::updateGraphs(
	const std::map<const CGHeroInstance *, HeroRole> & heroes,
	uint8_t mainScanDepth,
	uint8_t scoutScanDepth)
{
	auto start = std::chrono::high_resolution_clock::now();
	std::vector<const CGHeroInstance *> heroesVector;

	heroGraphs.clear();

	for(auto hero : heroes)
	{
		if(heroGraphs.try_emplace(hero.first->id).second)
		{
			heroGraphs[hero.first->id] = std::make_unique<GraphPaths>();
			heroesVector.push_back(hero.first);
		}
	}

	tbb::parallel_for(tbb::blocked_range<size_t>(0, heroesVector.size()), [this, &heroesVector, &heroes, mainScanDepth, scoutScanDepth](const tbb::blocked_range<size_t> & r)
		{
			for(auto i = r.begin(); i != r.end(); i++)
			{
				auto role = heroes.at(heroesVector[i]);
				auto scanLimit = role == HeroRole::MAIN ? mainScanDepth : scoutScanDepth;

				heroGraphs.at(heroesVector[i]->id)->calculatePaths(heroesVector[i], aiNk, scanLimit);
			}
		});

	if(NK2AI_GRAPH_TRACE_LEVEL >= 1)
	{
		for(auto hero : heroes)
		{
			heroGraphs[hero.first->id]->dumpToLog();
		}
	}

	logAi->trace("Graph paths updated in %lld", timeElapsed(start));
}

}
