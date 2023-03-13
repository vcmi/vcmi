/*
* DangerHitMapAnalyzer.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "../StdInc.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "../Engine/Nullkiller.h"

namespace NKAI
{

HitMapInfo HitMapInfo::NoTreat;

void DangerHitMapAnalyzer::updateHitMap()
{
	if(upToDate)
		return;

	logAi->trace("Update danger hitmap");

	upToDate = true;
	auto start = std::chrono::high_resolution_clock::now();

	auto cb = ai->cb.get();
	auto mapSize = ai->cb->getMapSize();
	hitMap.resize(boost::extents[mapSize.x][mapSize.y][mapSize.z]);
	enemyHeroAccessibleObjects.clear();

	std::map<PlayerColor, std::map<const CGHeroInstance *, HeroRole>> heroes;

	for(const CGObjectInstance * obj : ai->memory->visitableObjs)
	{
		if(obj->ID == Obj::HERO)
		{
			auto hero = dynamic_cast<const CGHeroInstance *>(obj);

			heroes[hero->tempOwner][hero] = HeroRole::MAIN;
		}
	}

	foreach_tile_pos([&](const int3 & pos){
		hitMap[pos.x][pos.y][pos.z].reset();
	});

	for(auto pair : heroes)
	{
		if(!pair.first.isValidPlayer())
			continue;

		if(ai->cb->getPlayerRelations(ai->playerID, pair.first) != PlayerRelations::ENEMIES)
			continue;

		ai->pathfinder->updatePaths(pair.second, PathfinderSettings());

		boost::this_thread::interruption_point();

		pforeachTilePos(mapSize, [&](const int3 & pos)
		{
			for(AIPath & path : ai->pathfinder->getPathInfo(pos))
			{
				if(path.getFirstBlockedAction())
					continue;

				auto tileDanger = path.getHeroStrength();
				auto turn = path.turn();
				auto & node = hitMap[pos.x][pos.y][pos.z];

				if(tileDanger / (turn + 1) > node.maximumDanger.danger / (node.maximumDanger.turn + 1)
					|| (tileDanger == node.maximumDanger.danger && node.maximumDanger.turn > turn))
				{
					node.maximumDanger.danger = tileDanger;
					node.maximumDanger.turn = turn;
					node.maximumDanger.hero = path.targetHero;
				}

				if(turn < node.fastestDanger.turn
					|| (turn == node.fastestDanger.turn && node.fastestDanger.danger < tileDanger))
				{
					node.fastestDanger.danger = tileDanger;
					node.fastestDanger.turn = turn;
					node.fastestDanger.hero = path.targetHero;
				}

				if(turn == 0)
				{
					auto objects = cb->getVisitableObjs(pos, false);
					
					for(auto obj : objects)
					{
						if(cb->getPlayerRelations(obj->tempOwner, ai->playerID) != PlayerRelations::ENEMIES)
							enemyHeroAccessibleObjects[path.targetHero].insert(obj);
					}
				}
			}
		});
	}

	logAi->trace("Danger hit map updated in %ld", timeElapsed(start));
}

uint64_t DangerHitMapAnalyzer::enemyCanKillOurHeroesAlongThePath(const AIPath & path) const
{
	int3 tile = path.targetTile();
	int turn = path.turn();
	const HitMapNode & info = hitMap[tile.x][tile.y][tile.z];

	return (info.fastestDanger.turn <= turn && !isSafeToVisit(path.targetHero, path.heroArmy, info.fastestDanger.danger))
		|| (info.maximumDanger.turn <= turn && !isSafeToVisit(path.targetHero, path.heroArmy, info.maximumDanger.danger));
}

const HitMapNode & DangerHitMapAnalyzer::getObjectTreat(const CGObjectInstance * obj) const
{
	auto tile = obj->visitablePos();

	return getTileTreat(tile);
}

const HitMapNode & DangerHitMapAnalyzer::getTileTreat(const int3 & tile) const
{
	const HitMapNode & info = hitMap[tile.x][tile.y][tile.z];

	return info;
}

const std::set<const CGObjectInstance *> empty = {};

const std::set<const CGObjectInstance *> & DangerHitMapAnalyzer::getOneTurnAccessibleObjects(const CGHeroInstance * enemy) const
{
	auto result = enemyHeroAccessibleObjects.find(enemy);
	
	if(result == enemyHeroAccessibleObjects.end())
	{
		return empty;
	}

	return result->second;
}

void DangerHitMapAnalyzer::reset()
{
	upToDate = false;
}

}
