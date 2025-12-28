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
#include "../pforeach.h"
#include "../../../lib/callback/GameRandomizer.h"
#include "../../../lib/logging/VisualLogger.h"

namespace NK2AI
{

const HitMapInfo HitMapInfo::NoThreat;

void logHitmap(PlayerColor playerID, DangerHitMapAnalyzer & data, const CCallback & cc)
{
#if NK2AI_TRACE_LEVEL >= 1
	logVisual->updateWithLock(
		playerID.toString() + ".danger.max",
		[&data, &cc](IVisualLogBuilder & b)
		{
			foreach_tile_pos(
				cc,
				[&b, &data](const int3 & pos)
				{
					auto & threat = data.getTileThreat(pos).maximumDanger;
					b.addText(pos, std::to_string(threat.danger));

					if(threat.heroPtr.isVerified())
					{
						b.addText(pos, std::to_string(threat.turn));
						b.addText(pos, threat.heroPtr->getNameTranslated());
					}
				}
			);
		}
	);

	logVisual->updateWithLock(
		playerID.toString() + ".danger.fast",
		[&data, &cc](IVisualLogBuilder & b)
		{
			foreach_tile_pos(
				cc,
				[&b, &data](const int3 & pos)
				{
					auto & threat = data.getTileThreat(pos).fastestDanger;
					b.addText(pos, std::to_string(threat.danger));

					if(threat.heroPtr.isVerified())
					{
						b.addText(pos, std::to_string(threat.turn));
						b.addText(pos, threat.heroPtr->getNameTranslated());
					}
				}
			);
		}
	);
#endif
}

void DangerHitMapAnalyzer::updateHitMap()
{
	if(hitMapUpToDate)
		return;

	logAi->trace("Update danger hitmap");

	hitMapUpToDate = true;
	auto start = std::chrono::high_resolution_clock::now();

	auto cc = aiNk->cc.get();
	auto mapSize = aiNk->cc->getMapSize();
	
	if(hitMap.shape()[0] != mapSize.x || hitMap.shape()[1] != mapSize.y || hitMap.shape()[2] != mapSize.z)
		hitMap.resize(boost::extents[mapSize.x][mapSize.y][mapSize.z]);

	enemyHeroAccessibleObjects.clear();
	townThreats.clear();

	std::map<PlayerColor, std::map<const CGHeroInstance *, HeroRole>> heroes;

	for(const ObjectInstanceID objId : aiNk->memory->visitableObjs)
	{
		const CGObjectInstance * obj = cc->getObjInstance(objId);
		if(!obj)
			continue;

		if(obj->ID == Obj::HERO)
		{
			auto hero = dynamic_cast<const CGHeroInstance *>(obj);

			heroes[hero->tempOwner][hero] = HeroRole::MAIN;
		}
		if(obj->ID == Obj::TOWN)
		{
			auto town = dynamic_cast<const CGTownInstance *>(obj);

			if(town->getGarrisonHero())
				heroes[town->getGarrisonHero()->tempOwner][town->getGarrisonHero()] = HeroRole::MAIN;
		}
	}

	auto ourTowns = cc->getTownsInfo();

	for(auto town : ourTowns)
	{
		townThreats[town->id]; // insert empty list
	}

	foreach_tile_pos(*aiNk->cc, [&](const int3 & pos){
		hitMap[pos.x][pos.y][pos.z].reset();
	});

	for(auto pair : heroes)
	{
		if(!pair.first.isValidPlayer())
			continue;

		if(aiNk->cc->getPlayerRelations(aiNk->playerID, pair.first) != PlayerRelations::ENEMIES)
			continue;

		PathfinderSettings ps;

		ps.scoutTurnDistanceLimit = ps.mainTurnDistanceLimit = aiNk->settings->getThreatTurnDistanceLimit();
		ps.useHeroChain = false;

		aiNk->pathfinder->updatePaths(pair.second, ps);

		aiNk->makingTurnInterruption.interruptionPoint();

		pforeachTilePaths(mapSize, aiNk, [&](const int3 & pos, const std::vector<AIPath> & paths)
		{
			for(const AIPath & path : paths)
			{
				if(path.getFirstBlockedAction())
					continue;

				auto & node = hitMap[pos.x][pos.y][pos.z];

				HitMapInfo newThreat;

				newThreat.heroPtr = HeroPtr(path.targetHero, aiNk->cc.get());
				newThreat.turn = path.turn();
				newThreat.threat = path.getHeroStrength() * (1 - path.movementCost() / 2.0);
				// TODO: Mircea: Why is this danger calculated so differently than FuzzyHelper::evaluateDanger?
				// shouldn't it use the path danger instead of hero strength?
				newThreat.danger = path.getHeroStrength();
#if NK2AI_TRACE_LEVEL >= 1
				auto danger2 = aiNk->dangerEvaluator->evaluateDanger(pos, path.targetHero);
				logAi->trace("Danger comparison for (%d %d %d) %s is %f vs %f",
					pos.x, pos.y, pos.z, path.targetHero->getNameTranslated(), newThreat.danger, danger2);
#endif
				if(newThreat.value() > node.maximumDanger.value())
				{
					node.maximumDanger = newThreat;
				}

				if(newThreat.turn < node.fastestDanger.turn
					|| (newThreat.turn == node.fastestDanger.turn && node.fastestDanger.danger < newThreat.danger))
				{
					node.fastestDanger = newThreat;
				}

				auto objects = cc->getVisitableObjs(pos, false);

				for(auto obj : objects)
				{
					if(obj->ID == Obj::TOWN && obj->getOwner() == aiNk->playerID)
					{
						auto & threats = townThreats[obj->id];
						auto threat = std::find_if(threats.begin(), threats.end(), [&](const HitMapInfo & i) -> bool
							{
								return i.heroPtr.idOrNone() == path.targetHero->id;
							});

						if(threat == threats.end())
						{
							threats.emplace_back();
							threat = std::prev(threats.end(), 1);
						}

						if(newThreat.value() > threat->value())
						{
							*threat = newThreat;
						}

						if(newThreat.turn == 0)
						{
							if(cc->getPlayerRelations(obj->tempOwner, aiNk->playerID) != PlayerRelations::ENEMIES)
								enemyHeroAccessibleObjects.emplace_back(path.targetHero, obj);
						}
					}
				}
			}
		});
	}

	logAi->trace("Danger hit map updated in %ld", timeElapsed(start));

	logHitmap(aiNk->playerID, *this, *aiNk->cc);
}

void DangerHitMapAnalyzer::calculateTileOwners()
{
	if(tileOwnersUpToDate) return;

	tileOwnersUpToDate = true;

	auto * cc = aiNk->cc.get();
	auto mapSize = aiNk->cc->getMapSize();

	if(hitMap.shape()[0] != mapSize.x || hitMap.shape()[1] != mapSize.y || hitMap.shape()[2] != mapSize.z)
		hitMap.resize(boost::extents[mapSize.x][mapSize.y][mapSize.z]);

	std::vector<std::unique_ptr<CGHeroInstance>> temporaryHeroes;
	std::map<const CGHeroInstance *, const CGTownInstance *> heroTownMap;
	std::map<const CGHeroInstance *, HeroRole> townHeroes;

	auto addTownHero = [&](const CGTownInstance * town)
	{
			auto *townHero = temporaryHeroes.emplace_back(std::make_unique<CGHeroInstance>(town->cb)).get();
			GameRandomizer randomizer(*town->cb);
			auto visitablePos = town->visitablePos();
			
			townHero->id = town->id;
			townHero->setOwner(aiNk->playerID); // lets avoid having multiple colors
			townHero->initHero(randomizer, static_cast<HeroTypeID>(0), true);
			townHero->pos = townHero->convertFromVisitablePos(visitablePos);
			townHero->initObj(randomizer);
			
			heroTownMap[townHero] = town;
			townHeroes[townHero] = HeroRole::MAIN;
	};

	for(const ObjectInstanceID objId : aiNk->memory->visitableObjs)
	{
		const CGObjectInstance * obj = cc->getObjInstance(objId);
		if(obj && obj->ID == Obj::TOWN)
			addTownHero(dynamic_cast<const CGTownInstance *>(obj));
	}

	for(const auto *town : cc->getTownsInfo())
	{
		addTownHero(town);
	}

	PathfinderSettings ps;
	ps.mainTurnDistanceLimit = ps.scoutTurnDistanceLimit = aiNk->settings->getMainHeroTurnDistanceLimit();

	aiNk->pathfinder->updatePaths(townHeroes, ps);

	pforeachTilePaths(mapSize, aiNk, [&](const int3 & pos, const std::vector<AIPath> & paths)
		{
			float ourDistance = std::numeric_limits<float>::max();
			float enemyDistance = std::numeric_limits<float>::max();
			const CGTownInstance * enemyTown = nullptr;
			const CGTownInstance * ourTown = nullptr;

			for(const AIPath & path : paths)
			{
				if(!path.targetHero || path.getFirstBlockedAction())
					continue;

				const auto *town = heroTownMap[path.targetHero];

				if(town->getOwner() == aiNk->playerID)
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

			if(vstd::isAlmostEqual(ourDistance, enemyDistance))
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

const std::vector<HitMapInfo> & DangerHitMapAnalyzer::getTownThreats(const CGTownInstance * town) const
{
	static const std::vector<HitMapInfo> empty = {};

	auto result = townThreats.find(town->id);

	return result == townThreats.end() ? empty : result->second;
}

PlayerColor DangerHitMapAnalyzer::getTileOwner(const int3 & tile) const
{
	const auto *town = hitMap[tile.x][tile.y][tile.z].closestTown;

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

	const auto& info = getTileThreat(tile);
	
	return (info.fastestDanger.turn <= turn && !isSafeToVisit(path.targetHero, path.heroArmy, info.fastestDanger.danger, aiNk->settings->getSafeAttackRatio()))
		|| (info.maximumDanger.turn <= turn && !isSafeToVisit(path.targetHero, path.heroArmy, info.maximumDanger.danger, aiNk->settings->getSafeAttackRatio()));
}

const HitMapNode & DangerHitMapAnalyzer::getObjectThreat(const CGObjectInstance * obj) const
{
	auto tile = obj->visitablePos();
	return getTileThreat(tile);
}

const HitMapNode & DangerHitMapAnalyzer::getTileThreat(const int3 & tile) const
{
	return hitMap[tile.x][tile.y][tile.z];
}

std::set<const CGObjectInstance *> DangerHitMapAnalyzer::getOneTurnAccessibleObjects(const CGHeroInstance * enemy) const
{
	std::set<const CGObjectInstance *> result;

	for(const auto & obj : enemyHeroAccessibleObjects)
	{
		if(obj.hero == enemy)
			result.insert(obj.obj);
	}

	return result;
}

}
