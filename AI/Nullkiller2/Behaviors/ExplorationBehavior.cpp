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
		const bool reachableByHero = vstd::contains_if(paths, [hero, aiNk](const AIPath & path)
		{
			return path.targetHero == hero
				&& path.exchangeCount <= 1
				&& !path.getFirstBlockedAction()
				&& isSafeToVisit(hero, path.heroArmy, path.getTotalDanger(), aiNk->settings->getSafeAttackRatio());
		});
		if(reachableByHero)
			return true;
	}

	ExplorationHelper helper(hero, aiNk);
	return helper.scanMap();
}

/// Tells whether a hero has nothing left to do on its side of the portal
using IdleHeroCheck = std::function<bool(const CGHeroInstance *)>;

bool isBetterPortalPath(const AIPath & candidate, const AIPath & current)
{
	if(candidate.turn() != current.turn())
		return candidate.turn() < current.turn();

	if(!vstd::isAlmostEqual(candidate.movementCost(), current.movementCost()))
		return candidate.movementCost() < current.movementCost();

	return candidate.targetHero->estimateHeroCombatValue() < current.targetHero->estimateHeroCombatValue();
}

bool isBetterPortalReinforcementPath(const AIPath & candidate, const AIPath & current)
{
	const uint64_t candidateStrength = getHeroFightingStrength(candidate.targetHero, candidate.heroArmy);
	const uint64_t currentStrength = getHeroFightingStrength(current.targetHero, current.heroArmy);
	if(candidateStrength != currentStrength)
		return candidateStrength > currentStrength;

	return isBetterPortalPath(candidate, current);
}

std::optional<uint64_t> getStrongestUnreturnedPortalHeroStrength(
	const CGObjectInstance * entrance,
	const Nullkiller * aiNk)
{
	std::optional<uint64_t> result;
	for(const ObjectInstanceID heroId : aiNk->memory->getUnreturnedOneWayPortalHeroes(entrance->id))
	{
		const auto * hero = aiNk->cc->getHero(heroId);
		if(!hero)
			continue;

		const uint64_t strength = getHeroFightingStrength(hero, hero);
		if(!result || strength > *result)
			result = strength;
	}
	return result;
}

std::optional<AIPath> findBestPortalPath(
	const std::vector<AIPath> & paths,
	const Nullkiller * aiNk,
	const IdleHeroCheck & isIdle,
	std::optional<HeroRole> requiredRole,
	std::optional<ObjectInstanceID> requiredHero,
	std::optional<uint64_t> minimumStrength = std::nullopt,
	bool idleOnly = false)
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

		if(const auto blockedAction = path.getFirstBlockedAction();
			blockedAction && blockedAction->decompose(aiNk, hero)->invalid())
		{
			logAi->trace(
				"One-way portal candidate %s rejected: route requires an unsupported blocked special action",
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

		const bool soleHero = isSoleNoTownHero(hero, aiNk);
		const bool idle = (idleOnly || soleHero) && isIdle(hero);
		if(idleOnly && !idle)
		{
			logAi->trace(
				"One-way portal candidate %s rejected: hero still has useful tasks on this side",
				hero->getNameTextID());
			continue;
		}

		const bool desperation = soleHero && idle;
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

		const uint64_t heroStrength = getHeroFightingStrength(hero, path.heroArmy);
		if(minimumStrength && heroStrength <= *minimumStrength)
		{
			logAi->trace(
				"One-way portal reinforcement candidate %s rejected: strength %lld does not exceed expedition strength %lld",
				hero->getNameTextID(),
				static_cast<long long>(heroStrength),
				static_cast<long long>(*minimumStrength));
			continue;
		}

		if(!result
			|| (minimumStrength
				? isBetterPortalReinforcementPath(path, *result)
				: isBetterPortalPath(path, *result)))
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

/// True if every known exit is in sight, a free one exists and none of them lands the hero next to a threat it can not beat
bool isPortalLandingSafe(
	const CGObjectInstance * entrance,
	const AIPath & path,
	const Nullkiller * aiNk)
{
	const auto * teleport = dynamic_cast<const CGTeleport *>(entrance);
	if(!teleport)
		return false;

	bool hasFreeExit = false;
	for(const ObjectInstanceID exitId : aiNk->cc->getTeleportChannelExits(teleport->channel))
	{
		const auto * exit = aiNk->cc->getObj(exitId, false);
		if(!exit || !aiNk->cc->isVisible(exit->visitablePos()))
			return false;

		// an own hero standing on the exit makes it impassable
		const auto * occupant = findObjectOfType(aiNk->cc->getVisitableObjs(exit->visitablePos(), false), Obj::HERO);
		if(occupant && occupant->getOwner() == aiNk->playerID)
			continue;

		if(!isSafeToVisit(
			path.targetHero,
			path.heroArmy,
			aiNk->dangerEvaluator->evaluateDanger(exit->visitablePos(), path.targetHero),
			aiNk->settings->getSafeAttackRatio()))
		{
			return false;
		}

		hasFreeExit = true;
	}
	return hasFreeExit;
}

std::optional<AIPath> findPortalProbePath(
	const CGObjectInstance * entrance,
	const Nullkiller * aiNk,
	std::optional<uint64_t> minimumStrength,
	bool safeLandingOnly,
	const IdleHeroCheck & isIdle)
{
	auto paths = aiNk->pathfinder->getPathInfo(
		entrance->visitablePos(),
		aiNk->isObjectGraphAllowed());
	if(safeLandingOnly)
	{
		vstd::erase_if(paths, [&](const AIPath & path)
		{
			return !path.targetHero || !isPortalLandingSafe(entrance, path, aiNk);
		});
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
				reservedHero->getNameTextID());
			return std::nullopt;
		}

		if(auto reservedPath = findBestPortalPath(
			paths,
			aiNk,
			isIdle,
			std::nullopt,
			reservation,
			minimumStrength))
		{
			return reservedPath;
		}

		logAi->debug(
			"Clearing stale one-way portal %d reservation for hero %d",
			entrance->id.getNum(),
			reservation->getNum());
		aiNk->memory->clearOneWayPortalReservation(entrance->id);
	}

	// a blind probe is a job for scouts; a main hero only goes when it has nothing left to do on this side
	const bool blindProbe = !minimumStrength;
	if(blindProbe)
	{
		if(auto scoutPath = findBestPortalPath(paths, aiNk, isIdle, HeroRole::SCOUT, std::nullopt))
			return scoutPath;
	}

	return findBestPortalPath(
		paths,
		aiNk,
		isIdle,
		HeroRole::MAIN,
		std::nullopt,
		minimumStrength,
		blindProbe);
}

/// Objects that only the heroes already sent through the portal can reach
std::vector<const CGObjectInstance *> getObjectsBehindPortal(
	const CGObjectInstance * entrance,
	const Nullkiller * aiNk)
{
	std::vector<const CGObjectInstance *> result;
	const auto expedition = aiNk->memory->getUnreturnedOneWayPortalHeroes(entrance->id);
	if(expedition.empty())
		return result;

	const auto heroes = aiNk->cc->getHeroesInfo();
	for(const ObjectInstanceID objectId : aiNk->memory->visitableObjs)
	{
		const auto * object = aiNk->cc->getObj(objectId, false);
		if(!object || !aiNk->cc->isVisible(object->visitablePos()))
			continue;

		bool reachableByExpedition = false;
		bool reachableFromHome = false;
		for(const auto * hero : heroes)
		{
			if(!aiNk->pathfinder->isTileAccessible(HeroPtr(hero, aiNk->cc.get()), object->visitablePos()))
				continue;

			if(vstd::contains(expedition, hero->id))
				reachableByExpedition = true;
			else
				reachableFromHome = true;
		}

		if(reachableByExpedition && !reachableFromHome)
			result.push_back(object);
	}
	return result;
}

/// Main hero route through an already scouted portal, taken for the sake of the objects behind it
std::optional<AIPath> findPortalCrossingPath(
	const CGObjectInstance * entrance,
	const std::vector<const CGObjectInstance *> & objectsBehind,
	const Nullkiller * aiNk,
	const IdleHeroCheck & isIdle)
{
	// without a way back the hero needs a town on the other side to keep fighting
	const bool canSettle = aiNk->memory->hasKnownOneWayPortalReturn(entrance->id)
		|| findObjectOfType(objectsBehind, Obj::TOWN);
	if(!canSettle)
		return std::nullopt;

	const auto paths = aiNk->pathfinder->getPathInfo(
		entrance->visitablePos(),
		aiNk->isObjectGraphAllowed());
	const auto path = findBestPortalPath(paths, aiNk, isIdle, HeroRole::MAIN, std::nullopt);
	if(!path || !isPortalLandingSafe(entrance, *path, aiNk))
		return std::nullopt;

	return path;
}

/// Shortest step off the exit onto a plain unguarded tile, so that other heroes can arrive through the portal
std::optional<AIPath> findExitVacatingPath(
	const CGHeroInstance * hero,
	const CGObjectInstance * exit,
	const Nullkiller * aiNk)
{
	std::optional<AIPath> result;
	if(hero->visitablePos() != exit->visitablePos())
		return result;

	for(const int3 & direction : int3::getDirs())
	{
		const int3 destination = hero->visitablePos() + direction;
		if(!aiNk->cc->isInTheMap(destination)
			|| aiNk->cc->getGuardingCreaturePosition(destination).isValid()
			|| !aiNk->cc->getVisitableObjs(destination, false).empty())
		{
			continue;
		}

		for(const auto & path : aiNk->pathfinder->getPathInfo(destination, false))
		{
			if(path.targetHero != hero
				|| path.exchangeCount > 1
				|| path.turn() != 0
				|| path.getTotalDanger() != 0
				|| path.getFirstBlockedAction())
			{
				continue;
			}

			if(!result || path.movementCost() < result->movementCost())
				result = path;
		}
	}
	return result;
}

std::optional<AIPath> findEntranceGuardPath(
	const CGObjectInstance * entrance,
	const CGObjectInstance * guard,
	const Nullkiller * aiNk,
	const IdleHeroCheck & isIdle)
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

		auto paths = aiNk->pathfinder->getPathInfo(
			destination,
			aiNk->isObjectGraphAllowed());
		// the guard task carries no prerequisite, so its route must be executable as is
		vstd::erase_if(paths, [](const AIPath & path)
		{
			return path.getFirstBlockedAction() != nullptr;
		});
		const auto path = findBestPortalPath(
			paths,
			aiNk,
			isIdle,
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
		if(!exit)
			continue;

		if(!guard)
		{
			if(const auto stepAside = findExitVacatingPath(hero, exit, aiNk))
			{
				logAi->debug(
					"One-way portal intent: hero %s frees exit %d",
					hero->getNameTextID(),
					exit->id.getNum());
				tasks.push_back(sptr(Composition()
					.addNext(OneWayPortalProbe(exit->visitablePos()))
					.addNext(ExecuteHeroChain(*stepAside))));
			}
			continue;
		}

		// the guard is visible, so a lost fight reveals nothing: a hero that can not win stays on the exit,
		// only a hero with nothing to lose attacks regardless
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

		// the check walks every known object, so each hero is evaluated at most once per entrance
		std::map<const CGHeroInstance *, bool> idleHeroes;
		const IdleHeroCheck isIdle = [&](const CGHeroInstance * hero)
		{
			const auto [cached, inserted] = idleHeroes.try_emplace(hero);
			if(inserted)
				cached->second = !hasAlternativeProgress(hero, entrance, aiNk);
			return cached->second;
		};

		// a sole hero is the only candidate, so this also describes whichever hero ends up probing
		const bool desperation = vstd::contains_if(aiNk->cc->getHeroesInfo(), [&](const CGHeroInstance * hero)
		{
			return isSoleNoTownHero(hero, aiNk) && isIdle(hero);
		});

		if(const auto * guard = getVisibleEntranceGuard(entrance, aiNk))
		{
			aiNk->memory->clearOneWayPortalReservation(entrance->id);
			const auto mainPath = findEntranceGuardPath(entrance, guard, aiNk, isIdle);
			if(mainPath)
			{
				logAi->debug(
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

		if(aiNk->memory->hasActiveOneWayPortalJourney(entrance->id)
			&& !desperation)
		{
			logAi->debug(
				"Suppressing repeated traffic through one-way portal %d: an expedition is still active",
				entrance->id.getNum());
			continue;
		}

		const bool probed = aiNk->memory->wasOneWayPortalProbed(entrance->id);
		const auto objectsBehind = probed
			? getObjectsBehindPortal(entrance, aiNk)
			: std::vector<const CGObjectInstance *>();
		std::optional<uint64_t> reinforcementStrength;
		bool probeAllowed = true;
		bool expeditionLost = false;

		if(probed
			&& !aiNk->memory->hasKnownOneWayPortalReturn(entrance->id)
			&& !desperation)
		{
			reinforcementStrength = getStrongestUnreturnedPortalHeroStrength(entrance, aiNk);
			// nobody is left to report from the other side, so the next hero relies on what is visible there
			expeditionLost = !reinforcementStrength;
			const bool enemyBehind = vstd::contains_if(objectsBehind, [aiNk](const CGObjectInstance * object)
			{
				return (object->ID == Obj::TOWN || object->ID == Obj::HERO)
					&& object->getOwner().isValidPlayer()
					&& aiNk->cc->getPlayerRelations(aiNk->playerID, object->getOwner()) == PlayerRelations::ENEMIES;
			});
			if(!expeditionLost && !enemyBehind)
			{
				logAi->debug(
					"Suppressing repeated traffic through one-way portal %d: no return route is known",
					entrance->id.getNum());
				probeAllowed = false;
			}
		}

		auto path = probeAllowed
			? findPortalProbePath(entrance, aiNk, reinforcementStrength, expeditionLost, isIdle)
			: std::nullopt;

		// rewards behind a scouted portal can justify sending a main hero that is still busy on this side
		std::vector<ObjectInstanceID> rewardsBehind;
		if(!path && probed)
		{
			path = findPortalCrossingPath(entrance, objectsBehind, aiNk, isIdle);
			if(path)
			{
				for(const auto * object : objectsBehind)
				{
					if(shouldVisit(aiNk, path->targetHero, object))
						rewardsBehind.push_back(object->id);
				}
			}

			if(rewardsBehind.empty())
				path.reset();
		}

		if(!path)
		{
			logAi->trace(
				"One-way portal %d has no eligible scout or main probe",
				entrance->id.getNum());
			continue;
		}

		const auto * teleport = dynamic_cast<const CGTeleport *>(entrance);
		logAi->debug(
			"One-way portal intent: %s entrance %d channel %d with %s hero %s",
			desperation ? "desperation" : !rewardsBehind.empty() ? "cross" : reinforcementStrength ? "reinforce" : "probe",
			entrance->id.getNum(),
			teleport ? teleport->channel.getNum() : -1,
			aiNk->heroManager->getHeroRoleOrDefaultInefficient(path->targetHero) == HeroRole::SCOUT ? "scout" : "main",
			path->targetHero->getNameTextID());
		Composition composition;
		composition
			.addNext(OneWayPortalProbe(entrance->visitablePos(), path->targetHero, rewardsBehind))
			.addNext(ExecuteHeroChain(*path, entrance));

		if(const auto blockedAction = path->getFirstBlockedAction())
			composition.addNext(blockedAction->decompose(aiNk, path->targetHero));

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
