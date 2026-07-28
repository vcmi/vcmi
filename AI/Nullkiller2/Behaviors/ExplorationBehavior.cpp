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

BoatExplorationEvaluation evaluateBoatExplorationCandidate(const BoatExplorationCandidate & candidate)
{
	BoatExplorationEvaluation evaluation;

	if(!candidate.available || candidate.hiddenTilesDiscovered <= 0)
		return evaluation;

	evaluation.accepted = true;
	evaluation.explorationValue = candidate.hiddenTilesDiscovered;
	return evaluation;
}

namespace
{
int countHiddenTilesAround(const Nullkiller * aiNk, const int3 & pos, const int sightRadius)
{
	const auto * teamState = aiNk->cc->getPlayerTeam(aiNk->playerID);
	const auto & fow = teamState->fogOfWarMap;
	int result = 0;
	int3 tile(0, 0, pos.z);

	for(tile.x = pos.x - sightRadius; tile.x <= pos.x + sightRadius; ++tile.x)
	{
		for(tile.y = pos.y - sightRadius; tile.y <= pos.y + sightRadius; ++tile.y)
		{
			if(aiNk->cc->isInTheMap(tile)
				&& pos.dist2d(tile) - 0.5 < sightRadius
				&& !fow[tile])
			{
				++result;
			}
		}
	}

	return result;
}

std::optional<int> getBoatExplorationValue(const Nullkiller * aiNk, const CGObjectInstance * obj)
{
	if(obj->ID != Obj::BOAT)
		return std::nullopt;

	const CGObjectInstance * topObj = aiNk->cc->getTopObj(obj->visitablePos());
	BoatExplorationCandidate candidate;
	candidate.available = topObj && topObj->id == obj->id;

	if(!candidate.available)
		return std::nullopt;

	int bestValue = 0;
	for(const CGHeroInstance * hero : aiNk->cc->getHeroesInfo())
		bestValue = std::max(bestValue, countHiddenTilesAround(aiNk, obj->visitablePos(), hero->getSightRadius()));

	candidate.hiddenTilesDiscovered = bestValue;
	const auto evaluation = evaluateBoatExplorationCandidate(candidate);
	if(!evaluation.accepted)
		return std::nullopt;

	return evaluation.explorationValue;
}

bool isSoleNoTownHero(const CGHeroInstance * hero, const Nullkiller * aiNk)
{
	const auto heroes = aiNk->cc->getHeroesInfo();
	return aiNk->cc->getTownsInfo().empty()
		&& heroes.size() == 1
		&& heroes.front() == hero;
}

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

	return candidate.targetHero->getTotalStrength() < current.targetHero->getTotalStrength();
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
				hero->getNameTranslated());
			continue;
		}

		if(path.exchangeCount > 1)
		{
			logAi->trace(
				"One-way portal candidate %s rejected: hero chains are disabled for probes",
				hero->getNameTranslated());
			continue;
		}

		if(const auto blockedAction = path.getFirstBlockedAction();
			blockedAction && blockedAction->decompose(aiNk, hero)->invalid())
		{
			logAi->trace(
				"One-way portal candidate %s rejected: route requires an unsupported blocked special action",
				hero->getNameTranslated());
			continue;
		}

		if(path.turn() == 0 && !hero->movementPointsRemaining())
		{
			logAi->trace(
				"One-way portal candidate %s rejected: hero has no movement points",
				hero->getNameTranslated());
			continue;
		}

		const int turnLimit = role == HeroRole::SCOUT
			? aiNk->settings->getScoutHeroTurnDistanceLimit()
			: aiNk->settings->getMainHeroTurnDistanceLimit();
		if(path.turn() > turnLimit)
		{
			logAi->trace(
				"One-way portal candidate %s rejected: arrival in %d turns exceeds limit %d",
				hero->getNameTranslated(),
				path.turn(),
				turnLimit);
			continue;
		}

		const bool desperation = isPortalDesperationHero(hero, entrance, aiNk);
		if(hero->isMissionCritical() && !desperation)
		{
			logAi->trace(
				"One-way portal candidate %s rejected: hero is mission-critical",
				hero->getNameTranslated());
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
				hero->getNameTranslated(),
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
	const auto paths = aiNk->pathfinder->getPathInfo(
		entrance->visitablePos(),
		aiNk->isObjectGraphAllowed());
	if(paths.empty())
	{
		logAi->trace(
			"One-way portal %d at %s has no pathfinder route",
			entrance->id.getNum(),
			entrance->visitablePos().toString());
	}
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
				reservedHero->getNameTranslated());
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

		const auto paths = aiNk->pathfinder->getPathInfo(
			destination,
			aiNk->isObjectGraphAllowed());
		const auto path = findBestPortalPath(
			paths,
			aiNk,
			entrance,
			HeroRole::MAIN,
			std::nullopt);
		if(!path || getOneWayPortalEntranceInPath(*path, aiNk))
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
		const bool protectedHero = hero->isMissionCritical() && !isSoleNoTownHero(hero, aiNk);
		if(exit && guard && !protectedHero)
		{
			logAi->debug(
				"One-way portal intent: trapped hero %s must fight exit guard %d",
				hero->getNameTranslated(),
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
					mainPath->targetHero->getNameTranslated());
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
			path->targetHero->getNameTranslated());
		Composition composition;
		composition
			.addNext(OneWayPortalProbe(entrance->visitablePos()))
			.addNext(ExecuteHeroChain(*path, entrance));

		if(const auto blockedAction = path->getFirstBlockedAction())
		{
			const auto prerequisite = blockedAction->decompose(aiNk, path->targetHero);
			if(!prerequisite->invalid())
				composition.addNext(prerequisite);
		}

		tasks.push_back(sptr(composition));
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
			case Obj::BOAT:
			{
				if(auto explorationValue = getBoatExplorationValue(aiNk, obj))
				{
					tasks.push_back(sptr(Composition()
						.addNext(ExplorationPoint(obj->visitablePos(), *explorationValue))
						.addNext(CaptureObject(obj))));
				}
				break;
			}
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
				const bool strategicOnlyMove = neighbourTarget->tilesDiscovered == 0;
				tasks.push_back(sptr(Composition()
					.addNext(ExplorationPoint(neighbourTarget->tile, neighbourTarget->tilesDiscovered))
					.addNext(ExploreNeighbourTile(hero, strategicOnlyMove ? 1 : 5, strategicOnlyMove))));
			}
		}
	}

	return tasks;
}

}
