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
#include "DangerHitMapAnalyzer.h"

#include "../Engine/Nullkiller.h"

namespace NKAI
{

HitMapInfo HitMapInfo::NoTreat;

double HitMapInfo::value() const
{
	return danger / std::sqrt(turn / 3.0f + 1);
}

void DangerHitMapAnalyzer::updateHitMap()
{
	if(hitMapUpToDate)
		return;

	logAi->trace("Update danger hitmap");

	hitMapUpToDate = true;
	auto start = std::chrono::high_resolution_clock::now();

	auto cb = ai->cb.get();
	auto mapSize = ai->cb->getMapSize();
	
	if(hitMap.shape()[0] != mapSize.x || hitMap.shape()[1] != mapSize.y || hitMap.shape()[2] != mapSize.z)
		hitMap.resize(boost::extents[mapSize.x][mapSize.y][mapSize.z]);

	enemyHeroAccessibleObjects.clear();
	townTreats.clear();

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

				auto & node = hitMap[pos.x][pos.y][pos.z];

				HitMapInfo newTreat;

				newTreat.hero = path.targetHero;
				newTreat.turn = path.turn();
				newTreat.danger = path.getHeroStrength();

				if(newTreat.value() > node.maximumDanger.value())
				{
					node.maximumDanger = newTreat;
				}

				if(newTreat.turn < node.fastestDanger.turn
					|| (newTreat.turn == node.fastestDanger.turn && node.fastestDanger.danger < newTreat.danger))
				{
					node.fastestDanger = newTreat;
				}

				if(newTreat.turn == 0)
				{
					auto objects = cb->getVisitableObjs(pos, false);
					
					for(auto obj : objects)
					{
						if(cb->getPlayerRelations(obj->tempOwner, ai->playerID) != PlayerRelations::ENEMIES)
							enemyHeroAccessibleObjects[path.targetHero].insert(obj);

						if(obj->ID == Obj::TOWN && obj->getOwner() == ai->playerID)
						{
							auto & treats = townTreats[obj->id];
							auto treat = std::find_if(treats.begin(), treats.end(), [&](const HitMapInfo & i) -> bool
								{
									return i.hero.hid == path.targetHero->id;
								});

							if(treat == treats.end())
							{
								treats.emplace_back();
								treat = std::prev(treats.end(), 1);
							}

							if(newTreat.value() > treat->value())
							{
								*treat = newTreat;
							}
						}
					}
				}
			}
		});
	}

	logAi->trace("Danger hit map updated in %ld", timeElapsed(start));
}

void DangerHitMapAnalyzer::calculateTileOwners()
{
	if(tileOwnersUpToDate) return;

	tileOwnersUpToDate = true;

	auto cb = ai->cb.get();
	auto mapSize = ai->cb->getMapSize();

	if(hitMap.shape()[0] != mapSize.x || hitMap.shape()[1] != mapSize.y || hitMap.shape()[2] != mapSize.z)
		hitMap.resize(boost::extents[mapSize.x][mapSize.y][mapSize.z]);

	std::map<const CGHeroInstance *, HeroRole> townHeroes;
	std::map<const CGHeroInstance *, const CGTownInstance *> heroTownMap;
	PathfinderSettings pathfinderSettings;

	pathfinderSettings.mainTurnDistanceLimit = 5;

	auto addTownHero = [&](const CGTownInstance * town)
	{
			auto townHero = new CGHeroInstance();
			CRandomGenerator rng;
			auto visitablePos = town->visitablePos();
			
			townHero->pos = visitablePos;
			townHero->setOwner(ai->playerID); // lets avoid having multiple colors
			townHero->initHero(rng, static_cast<HeroTypeID>(0));
			townHero->initObj(rng);
			townHero->pos = townHero->convertFromVisitablePos(visitablePos);
			
			heroTownMap[townHero] = town;
			townHeroes[townHero] = HeroRole::MAIN;
	};

	for(auto obj : ai->memory->visitableObjs)
	{
		if(obj && obj->ID == Obj::TOWN)
		{
			addTownHero(dynamic_cast<const CGTownInstance *>(obj));
		}
	}

	for(auto town : cb->getTownsInfo())
	{
		addTownHero(town);
	}

	ai->pathfinder->updatePaths(townHeroes, PathfinderSettings());

	pforeachTilePos(mapSize, [&](const int3 & pos)
		{
			float ourDistance = std::numeric_limits<float>::max();
			float enemyDistance = std::numeric_limits<float>::max();
			const CGTownInstance * enemyTown = nullptr;
			const CGTownInstance * ourTown = nullptr;

			for(AIPath & path : ai->pathfinder->getPathInfo(pos))
			{
				if(!path.targetHero || path.getFirstBlockedAction())
					continue;

				auto town = heroTownMap[path.targetHero];

				if(town->getOwner() == ai->playerID)
				{
					if(ourDistance > path.movementCost())
					{
						ourDistance = path.movementCost();
						ourTown = town;
					}
				}
				else
				{
					if(enemyDistance > path.movementCost())
					{
						enemyDistance = path.movementCost();
						enemyTown = town;
					}
				}
			}

			if(ourDistance == enemyDistance)
			{
				hitMap[pos.x][pos.y][pos.z].closestTown = nullptr;
			}
			else if(!enemyTown || ourDistance < enemyDistance)
			{
				hitMap[pos.x][pos.y][pos.z].closestTown = ourTown;
			}
			else
			{
				hitMap[pos.x][pos.y][pos.z].closestTown = enemyTown;
			}
		});
}

const std::vector<HitMapInfo> & DangerHitMapAnalyzer::getTownTreats(const CGTownInstance * town) const
{
	static const std::vector<HitMapInfo> empty = {};

	auto result = townTreats.find(town->id);

	return result == townTreats.end() ? empty : result->second;
}

PlayerColor DangerHitMapAnalyzer::getTileOwner(const int3 & tile) const
{
	auto town = hitMap[tile.x][tile.y][tile.z].closestTown;

	return town ? town->getOwner() : PlayerColor::NEUTRAL;
}

const CGTownInstance * DangerHitMapAnalyzer::getClosestTown(const int3 & tile) const
{
	return hitMap[tile.x][tile.y][tile.z].closestTown;
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
	hitMapUpToDate = false;
}

}
