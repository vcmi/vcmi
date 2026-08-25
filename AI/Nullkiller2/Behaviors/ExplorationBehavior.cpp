/*
* ExplorationBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../Goals/CaptureObject.h"
#include "../Goals/Composition.h"
#include "../Goals/AttackOneWayPortalGuard.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/ExploreNeighbourTile.h"
#include "../Goals/Invalid.h"
#include "../Helpers/ExplorationHelper.h"
#include "../Markers/ExplorationPoint.h"
#include "../Markers/OneWayPortalProbe.h"
#include "ExplorationBehavior.h"

#include "../../../lib/CPlayerState.h"

namespace NK2AI
{

using namespace Goals;

namespace
{
bool hasAlternativeProgress(
	const CGHeroInstance * hero,
	const CGObjectInstance * entrance,
	const Nullkiller * aiNk)
{
	for(const ObjectInstanceID objectId : aiNk->memory->visitableObjs)
	{
		const auto * object = aiNk->cc->getObj(objectId, false);
		if(!object || object->id == entrance->id || !shouldVisit(aiNk, hero, object))
			continue;

		const auto paths = aiNk->pathfinder->getPathInfo(object->visitablePos(), false);
		const bool reachableByHero = vstd::contains_if(paths, [hero](const AIPath & path)
		{
			return path.targetHero == hero
				&& path.exchangeCount <= 1
				&& !path.getFirstBlockedAction();
		});
		if(reachableByHero)
			return true;
	}

	ExplorationHelper helper(hero, aiNk);
	return helper.scanMap();
}

bool isPortalDesperationHero(
	const CGHeroInstance * hero,
	const CGObjectInstance * entrance,
	const Nullkiller * aiNk)
{
	return isSoleNoTownHero(hero, aiNk)
		&& !hasAlternativeProgress(hero, entrance, aiNk);
}

bool isBetterPortalPath(const AIPath & candidate, const AIPath & current)
{
	if(candidate.turn() != current.turn())
		return candidate.turn() < current.turn();

	if(!vstd::isAlmostEqual(candidate.movementCost(), current.movementCost()))
		return candidate.movementCost() < current.movementCost();

	return candidate.targetHero->estimateHeroCombatValue() < current.targetHero->estimateHeroCombatValue();
}

std::optional<AIPath> findBestPortalPath(
	const std::vector<AIPath> & paths,
	const Nullkiller * aiNk,
	const CGObjectInstance * entrance,
	std::optional<HeroRole> requiredRole,
	std::optional<ObjectInstanceID> requiredHero)
{
	std::optional<AIPath> result;

	for(const auto & path : paths)
	{
		const auto * hero = path.targetHero;
		if(!hero || hero->getOwner() != aiNk->playerID)
			continue;

		const auto role = aiNk->heroManager->getHeroRoleOrDefaultInefficient(hero);
		if(requiredRole && role != *requiredRole)
			continue;

		if(requiredHero && hero->id != *requiredHero)
			continue;

		if(aiNk->isHeroLocked(hero))
		{
			logAi->trace(
				"One-way portal candidate %s rejected: hero is locked",
				hero->getNameTextID());
			continue;
		}

		if(path.exchangeCount > 1)
		{
			logAi->trace(
				"One-way portal candidate %s rejected: hero chains are disabled for probes",
				hero->getNameTextID());
			continue;
		}

		if(path.getFirstBlockedAction())
		{
			logAi->trace(
				"One-way portal candidate %s rejected: route requires a blocked special action",
				hero->getNameTextID());
			continue;
		}

		if(path.turn() == 0 && !hero->movementPointsRemaining())
		{
			logAi->trace(
				"One-way portal candidate %s rejected: hero has no movement points",
				hero->getNameTextID());
			continue;
		}

		const int turnLimit = role == HeroRole::SCOUT
			? aiNk->settings->getScoutHeroTurnDistanceLimit()
			: aiNk->settings->getMainHeroTurnDistanceLimit();
		if(path.turn() > turnLimit)
		{
			logAi->trace(
				"One-way portal candidate %s rejected: arrival in %d turns exceeds limit %d",
				hero->getNameTextID(),
				path.turn(),
				turnLimit);
			continue;
		}

		const bool desperation = isPortalDesperationHero(hero, entrance, aiNk);
		if(hero->isMissionCritical() && !desperation)
		{
			logAi->trace(
				"One-way portal candidate %s rejected: hero is mission-critical",
				hero->getNameTextID());
			continue;
		}

		if(!desperation && !isSafeToVisit(
			hero,
			path.heroArmy,
			path.getTotalDanger(),
			aiNk->settings->getSafeAttackRatio()))
		{
			logAi->trace(
				"One-way portal candidate %s rejected: route danger %lld is unsafe",
				hero->getNameTextID(),
				path.getTotalDanger());
			continue;
		}

		if(!result || isBetterPortalPath(path, *result))
			result = path;
	}

	return result;
}

const CGObjectInstance * getVisibleEntranceGuard(
	const CGObjectInstance * entrance,
	const Nullkiller * aiNk)
{
	const auto guards = aiNk->cc->getGuardingCreatures(entrance->visitablePos());
	const auto guard = std::find_if(guards.begin(), guards.end(), [aiNk](const CGObjectInstance * object)
	{
		return object->ID == Obj::MONSTER && aiNk->cc->isVisible(object->visitablePos());
	});

	return guard != guards.end() ? *guard : nullptr;
}

std::optional<AIPath> findPortalProbePath(
	const CGObjectInstance * entrance,
	const Nullkiller * aiNk)
{
	const auto paths = aiNk->pathfinder->getPathInfo(entrance->visitablePos(), false);
	const auto reservation = aiNk->memory->getOneWayPortalReservation(entrance->id);
	if(reservation)
	{
		const auto * reservedHero = aiNk->cc->getHero(*reservation);
		if(reservedHero
			&& (aiNk->getHeroLockedReason(reservedHero) == HeroLockedReason::HERO_CHAIN
				|| !reservedHero->movementPointsRemaining()))
		{
			logAi->trace(
				"Keeping one-way portal %d reserved for exhausted hero %s until the next turn",
				entrance->id.getNum(),
				reservedHero->getNameTextID());
			return std::nullopt;
		}

		if(auto reservedPath = findBestPortalPath(paths, aiNk, entrance, std::nullopt, reservation))
			return reservedPath;

		logAi->debug(
			"Clearing stale one-way portal %d reservation for hero %d",
			entrance->id.getNum(),
			reservation->getNum());
		aiNk->memory->clearOneWayPortalReservation(entrance->id);
	}

	if(auto scoutPath = findBestPortalPath(paths, aiNk, entrance, HeroRole::SCOUT, std::nullopt))
		return scoutPath;

	return findBestPortalPath(paths, aiNk, entrance, HeroRole::MAIN, std::nullopt);
}

std::optional<AIPath> findEntranceGuardPath(
	const CGObjectInstance * entrance,
	const CGObjectInstance * guard,
	const Nullkiller * aiNk)
{
	std::optional<AIPath> result;

	for(const int3 & direction : int3::getDirs())
	{
		const int3 destination = guard->visitablePos() + direction;
		if(!aiNk->cc->isInTheMap(destination)
			|| destination == entrance->visitablePos()
			|| aiNk->cc->getGuardingCreaturePosition(destination) != guard->visitablePos())
		{
			continue;
		}

		const auto paths = aiNk->pathfinder->getPathInfo(destination, false);
		const auto path = findBestPortalPath(
			paths,
			aiNk,
			entrance,
			HeroRole::MAIN,
			std::nullopt);
		if(!path)
			continue;

		if(!result || isBetterPortalPath(*path, *result))
			result = path;
	}

	return result;
}

void addOneWayPortalTasks(Goals::TGoalVec & tasks, const Nullkiller * aiNk)
{
	for(const auto * hero : aiNk->cc->getHeroesInfo())
	{
		const auto journey = aiNk->memory->getOneWayPortalJourney(hero->id);
		if(!journey)
			continue;

		const auto * exit = aiNk->cc->getObj(journey->second, false);
		const auto * guard = AttackOneWayPortalGuard::findTrappingGuard(hero, aiNk);
		if(!exit || !guard)
			continue;

		// the guard is visible, so a lost fight reveals nothing; only a hero with nothing to lose attacks regardless
		const bool safeFight = !hero->isMissionCritical() && isSafeToVisit(
			hero,
			aiNk->dangerEvaluator->evaluateDanger(guard),
			aiNk->settings->getSafeAttackRatio());
		if(safeFight || isSoleNoTownHero(hero, aiNk))
		{
			logAi->debug(
				"One-way portal intent: trapped hero %s must fight exit guard %d",
				hero->getNameTextID(),
				guard->id.getNum());
			tasks.push_back(sptr(AttackOneWayPortalGuard(hero, guard, exit)));
		}
	}

	for(const ObjectInstanceID objId : aiNk->memory->visitableObjs)
	{
		const auto * entrance = aiNk->cc->getObj(objId, false);
		if(!entrance
			|| entrance->ID != Obj::MONOLITH_ONE_WAY_ENTRANCE
			|| !aiNk->cc->isVisible(entrance->visitablePos()))
		{
			continue;
		}

		const bool desperation = vstd::contains_if(aiNk->cc->getHeroesInfo(), [aiNk, entrance](const CGHeroInstance * hero)
		{
			return isPortalDesperationHero(hero, entrance, aiNk);
		});

		if(const auto * guard = getVisibleEntranceGuard(entrance, aiNk))
		{
			aiNk->memory->clearOneWayPortalReservation(entrance->id);
			const auto mainPath = findEntranceGuardPath(entrance, guard, aiNk);
			if(mainPath)
			{
				logAi->info(
					"One-way portal intent: clear entrance guard %d with main hero %s",
					guard->id.getNum(),
					mainPath->targetHero->getNameTextID());
				tasks.push_back(sptr(Composition()
					.addNext(ExplorationPoint(entrance->visitablePos(), 1))
					.addNext(ExecuteHeroChain(*mainPath, guard))));
			}
			continue;
		}

		if(aiNk->memory->wasOneWayPortalProbedToday(
			entrance->id,
			aiNk->cc->getCalendar().getCurrentDay()))
		{
			logAi->debug(
				"Suppressing repeated traffic through one-way portal %d: already used this turn",
				entrance->id.getNum());
			continue;
		}

		if(aiNk->memory->wasOneWayPortalProbed(entrance->id)
			&& !aiNk->memory->hasKnownOneWayPortalReturn(entrance->id)
			&& !desperation)
		{
			logAi->debug(
				"Suppressing repeated traffic through one-way portal %d: no return route is known",
				entrance->id.getNum());
			continue;
		}

		const auto path = findPortalProbePath(entrance, aiNk);
		if(!path)
		{
			logAi->trace(
				"One-way portal %d has no eligible scout or main probe",
				entrance->id.getNum());
			continue;
		}

		const auto * teleport = dynamic_cast<const CGTeleport *>(entrance);
		const bool desperationProbe = isPortalDesperationHero(path->targetHero, entrance, aiNk);
		logAi->info(
			"One-way portal intent: %s entrance %d channel %d with %s hero %s",
			desperationProbe ? "desperation" : "probe",
			entrance->id.getNum(),
			teleport ? teleport->channel.getNum() : -1,
			aiNk->heroManager->getHeroRoleOrDefaultInefficient(path->targetHero) == HeroRole::SCOUT ? "scout" : "main",
			path->targetHero->getNameTextID());
		tasks.push_back(sptr(Composition()
			.addNext(OneWayPortalProbe(entrance->visitablePos()))
			.addNext(ExecuteHeroChain(*path, entrance))));
	}
}
}

std::string ExplorationBehavior::toString() const
{
	return "Explore";
}

Goals::TGoalVec ExplorationBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;

	addOneWayPortalTasks(tasks, aiNk);

	if(aiNk->isOpenMap())
		return tasks;

	for(const ObjectInstanceID objId : aiNk->memory->visitableObjs)
	{
		const CGObjectInstance * obj = aiNk->cc->getObjInstance(objId);
		if(!obj)
			continue;

		switch(obj->ID.num)
		{
			case Obj::REDWOOD_OBSERVATORY:
			case Obj::PILLAR_OF_FIRE:
			{
				auto rObj = dynamic_cast<const CRewardableObject *>(obj);
				if(!rObj->wasScouted(aiNk->playerID))
					tasks.push_back(sptr(Composition().addNext(ExplorationPoint(obj->visitablePos(), 200)).addNext(CaptureObject(obj))));
				break;
			}
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
			case Obj::WHIRLPOOL:
			{
				const auto tObj = dynamic_cast<const CGTeleport *>(obj);
				for(auto exit : aiNk->cc->getTeleportChannelExits(tObj->channel))
				{
					if(exit != tObj->id)
					{
						const CGObjectInstance * exitObj = aiNk->cc->getObjInstance(exit);

						// Please check for visible tile and not the visible object due to possible partial visibility
						// checking the object itself might incorrectly abort the exploration of the fog behind it
						if(exitObj && !aiNk->cc->isVisible(exitObj->visitablePos()))
						{
							tasks.push_back(sptr(Composition().addNext(ExplorationPoint(obj->visitablePos(), 50)).addNext(CaptureObject(obj))));
						}
					}
				}
				break;
			}
		}
	}

	const auto heroes = aiNk->cc->getHeroesInfo();
	for(const CGHeroInstance * hero : heroes)
	{
		if(aiNk->isHeroLocked(hero))
			continue;

		ExplorationHelper scanResult(hero, aiNk);
		const bool canUseDimensionDoor = scanResult.canUseDimensionDoor();
		auto improveWithDimensionDoor = [canUseDimensionDoor, &scanResult]()
		{
			return canUseDimensionDoor && scanResult.considerDimensionDoorExplorationTargets();
		};

		const bool foundNearbyTarget = scanResult.scanSector(1);

		if(foundNearbyTarget)
		{
			// Let DD compete with local walking exploration; otherwise any nearby fog
			// tile would hide a much stronger spell landing from the task selector.
			improveWithDimensionDoor();

			tasks.push_back(scanResult.makeComposition());
			continue;
		}

		const bool foundSectorTarget = scanResult.scanSector(30);
		const bool foundDimensionDoorTarget = improveWithDimensionDoor();

		if(foundSectorTarget || foundDimensionDoorTarget)
		{
			tasks.push_back(scanResult.makeComposition());
			continue;
		}

		if(aiNk->getScanDepth() == ScanDepth::ALL_FULL)
		{
			if(scanResult.scanMap())
			{
				tasks.push_back(scanResult.makeComposition());
			}
			// Last-resort scout movement: if no full-map exploration target survived,
			// spend remaining MP on safe adjacent fog instead of ending the turn idle.
			else if(const auto neighbourTarget = ExploreNeighbourTile::findTarget(hero, aiNk))
			{
				tasks.push_back(sptr(Composition()
					.addNext(ExplorationPoint(neighbourTarget->tile, neighbourTarget->tilesDiscovered))
					.addNext(ExploreNeighbourTile(hero, 5))));
			}
		}
	}

	return tasks;
}

}
