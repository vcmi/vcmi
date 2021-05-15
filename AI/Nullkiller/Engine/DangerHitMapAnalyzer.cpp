/*
* DangerHitMapAnalyzer.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "DangerHitMapAnalyzer.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

void DangerHitMapAnalyzer::updateHitMap()
{
	auto mapSize = cb->getMapSize();
	hitMap.resize(boost::extents[mapSize.x][mapSize.y][mapSize.z]);

	std::map<PlayerColor, std::vector<HeroPtr>> heroes;

	for(const CGObjectInstance * obj : ai->visitableObjs)
	{
		if(obj->ID == Obj::HERO)
		{
			HeroPtr hero = dynamic_cast<const CGHeroInstance *>(obj);

			heroes[hero->tempOwner].push_back(hero);
		}
	}

	foreach_tile_pos([&](const int3 & pos){
		hitMap[pos.x][pos.y][pos.z].reset();
	});

	for(auto pair : heroes)
	{
		ai->ah->updatePaths(pair.second, false);

		foreach_tile_pos([&](const int3 & pos){
			for(AIPath & path : ai->ah->getPathsToTile(pos))
			{
				auto tileDanger = path.getHeroStrength();
				auto turn = path.turn();
				auto & node = hitMap[pos.x][pos.y][pos.z];

				if(tileDanger > node.maximumDanger.danger
					|| tileDanger == node.maximumDanger.danger && node.maximumDanger.turn > turn)
				{
					node.maximumDanger.danger = tileDanger;
					node.maximumDanger.turn = turn;
				}

				if(turn < node.fastestDanger.turn
					|| turn == node.fastestDanger.turn && node.fastestDanger.danger < tileDanger)
				{
					node.fastestDanger.danger = tileDanger;
					node.fastestDanger.turn = turn;
				}
			}
		});
	}
}

uint64_t DangerHitMapAnalyzer::enemyCanKillOurHeroesAlongThePath(const AIPath & path) const
{
	int3 tile = path.targetTile();
	int turn = path.turn();
	const HitMapNode & info = hitMap[tile.x][tile.y][tile.z];

	return info.fastestDanger.turn <= turn && !isSafeToVisit(path.targetHero, path.heroArmy, info.fastestDanger.danger)
		|| info.maximumDanger.turn <= turn && !isSafeToVisit(path.targetHero, path.heroArmy, info.maximumDanger.danger);
}
