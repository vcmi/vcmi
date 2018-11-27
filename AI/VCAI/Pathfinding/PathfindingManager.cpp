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
#include "Goals/Goals.h"
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
}

void PathfindingManager::setAI(VCAI * AI)
{
	ai = AI;
}

Goals::TGoalVec PathfindingManager::howToVisitTile(int3 tile)
{
	Goals::TGoalVec result;

	auto heroes = cb->getHeroesInfo();

	for(auto hero : heroes)
	{
		vstd::concatenate(result, howToVisitTile(hero, tile));
	}

	return result;
}

Goals::TGoalVec PathfindingManager::howToVisitObj(ObjectIdRef obj)
{
	Goals::TGoalVec result;

	auto heroes = cb->getHeroesInfo();

	for(auto hero : heroes)
	{
		vstd::concatenate(result, howToVisitObj(hero, obj));
	}

	return result;
}

Goals::TGoalVec PathfindingManager::howToVisitTile(HeroPtr hero, int3 tile, bool allowGatherArmy)
{
	auto result = findPath(hero, tile, allowGatherArmy, [&](int3 firstTileToGet) -> Goals::TSubgoal
	{
		return sptr(Goals::VisitTile(firstTileToGet).sethero(hero).setisAbstract(true));
	});

	for(Goals::TSubgoal solution : result)
	{
		solution->setparent(sptr(Goals::VisitTile(tile).sethero(hero).setevaluationContext(solution->evaluationContext)));
	}

	return result;
}

Goals::TGoalVec PathfindingManager::howToVisitObj(HeroPtr hero, ObjectIdRef obj, bool allowGatherArmy)
{
	if(!obj)
	{
		return Goals::TGoalVec();
	}

	int3 dest = obj->visitablePos();

	auto result = findPath(hero, dest, allowGatherArmy, [&](int3 firstTileToGet) -> Goals::TSubgoal
	{
		if(obj->ID.num == Obj::HERO && obj->getOwner() == hero->getOwner())
			return sptr(Goals::VisitHero(obj->id.getNum()).sethero(hero).setisAbstract(true));
		else
			return sptr(Goals::VisitObj(obj->id.getNum()).sethero(hero).setisAbstract(true));
	});

	for(Goals::TSubgoal solution : result)
	{
		solution->setparent(sptr(Goals::VisitObj(obj->id.getNum()).sethero(hero).setevaluationContext(solution->evaluationContext)));
	}

	return result;
}

std::vector<AIPath> PathfindingManager::getPathsToTile(HeroPtr hero, int3 tile)
{
	return pathfinder->getPathInfo(hero, tile);
}

Goals::TGoalVec PathfindingManager::findPath(
	HeroPtr hero,
	crint3 dest,
	bool allowGatherArmy,
	const std::function<Goals::TSubgoal(int3)> doVisitTile)
{
	Goals::TGoalVec result;
	boost::optional<uint64_t> armyValueRequired;
	uint64_t danger;

	std::vector<AIPath> chainInfo = pathfinder->getPathInfo(hero, dest);

	logAi->trace("Trying to find a way for %s to visit tile %s", hero->name, dest.toString());

	for(auto path : chainInfo)
	{
		int3 firstTileToGet = path.firstTileToGet();

		logAi->trace("Path found size=%i, first tile=%s", path.nodes.size(), firstTileToGet.toString());

		if(firstTileToGet.valid() && ai->isTileNotReserved(hero.get(), firstTileToGet))
		{
			danger = path.getTotalDanger(hero);

			if(isSafeToVisit(hero, danger))
			{
				logAi->trace("It's safe for %s to visit tile %s with danger %s", hero->name, dest.toString(), std::to_string(danger));

				Goals::TSubgoal solution;

				if(path.specialAction)
				{
					solution = path.specialAction->whatToDo(hero);
				}
				else
				{
					solution = dest == firstTileToGet
						? doVisitTile(firstTileToGet)
						: clearWayTo(hero, firstTileToGet);
				}

				if(solution->invalid())
					continue;

				if(solution->evaluationContext.danger < danger)
					solution->evaluationContext.danger = danger;
				
				solution->evaluationContext.movementCost += path.movementCost();

				result.push_back(solution);

				continue;
			}

			if(!armyValueRequired || armyValueRequired > danger)
			{
				armyValueRequired = boost::make_optional(danger);
			}
		}
	}

	danger = armyValueRequired.get_value_or(0);

	if(allowGatherArmy && danger > 0)
	{
		//we need to get army in order to conquer that place
		logAi->trace("Gather army for %s, value=%s", hero->name, std::to_string(danger));
		result.push_back(sptr(Goals::GatherArmy(danger * SAFE_ATTACK_CONSTANT).sethero(hero).setisAbstract(true)));
	}

	return result;
}

Goals::TSubgoal PathfindingManager::clearWayTo(HeroPtr hero, int3 firstTileToGet)
{
	if(isBlockedBorderGate(firstTileToGet))
	{
		//FIXME: this way we'll not visit gate and activate quest :?
		return sptr(Goals::FindObj(Obj::KEYMASTER, cb->getTile(firstTileToGet)->visitableObjects.back()->subID));
	}

	auto topObj = cb->getTopObj(firstTileToGet);
	if(topObj)
	{

		if(vstd::contains(ai->reservedObjs, topObj) && !vstd::contains(ai->reservedHeroesMap[hero], topObj))
		{
			return sptr(Goals::Invalid());
		}

		if(topObj->ID == Obj::HERO && cb->getPlayerRelations(hero->tempOwner, topObj->tempOwner) != PlayerRelations::ENEMIES)
		{
			if(topObj != hero.get(true)) //the hero we want to free
			{
				logAi->error("%s stands in the way of %s", topObj->getObjectName(), hero->getObjectName());

				return sptr(Goals::Invalid());
			}
		}

		if(topObj->ID == Obj::QUEST_GUARD || topObj->ID == Obj::BORDERGUARD)
		{
			if(shouldVisit(hero, topObj))
			{
				//do NOT use VISIT_TILE, as tile with quets guard can't be visited
				return sptr(Goals::VisitObj(topObj->id.getNum()).sethero(hero));
			}

			//TODO: we should be able to return apriopriate quest here
			//ret.push_back(ai->questToGoal());
			//however, visiting obj for firts time will give us quest
			//do not access quets guard if we can't complete the quest
			logAi->trace("Can not visit this quest guard! Not ready!");
			return sptr(Goals::Invalid());
		}
	}

	return sptr(Goals::VisitTile(firstTileToGet).sethero(hero).setisAbstract(true));
}

void PathfindingManager::resetPaths()
{
	logAi->debug("AIPathfinder has been reseted.");
	pathfinder->clear();
}
