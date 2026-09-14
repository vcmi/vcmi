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
#include "../Goals/BuildBoat.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/ExploreNeighbourTile.h"
#include "../Goals/Invalid.h"
#include "../Goals/SaveResources.h"
#include "CaptureObjectsBehavior.h"
#include "../Helpers/ExplorationHelper.h"
#include "../Markers/ExplorationPoint.h"
#include "../Markers/OneWayPortalProbe.h"
#include "ExplorationBehavior.h"

#include "../../../lib/CPlayerState.h"
#include "../../../lib/mapping/TerrainTile.h"

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
struct WaterExplorationTarget
{
	int3 tile;
	int value = 0;
};

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

std::optional<WaterExplorationTarget> findWaterExplorationTarget(
	const Nullkiller * aiNk,
	const int3 & start,
	const int sightRadius)
{
	const auto * teamState = aiNk->cc->getPlayerTeam(aiNk->playerID);
	const auto & fow = teamState->fogOfWarMap;
	std::queue<int3> pending;
	std::set<int3> visited;
	pending.push(start);
	visited.insert(start);

	WaterExplorationTarget bestTarget;
	while(!pending.empty())
	{
		const int3 current = pending.front();
		pending.pop();

		if(!aiNk->cc->isInTheMap(current) || !fow[current])
			continue;

		const auto * tile = aiNk->cc->getTile(current, false);
		if(!tile || !tile->isWater())
			continue;

		const int explorationValue = countHiddenTilesAround(aiNk, current, sightRadius);
		if(explorationValue > bestTarget.value)
			bestTarget = {current, explorationValue};

		for(const int3 & direction : int3::getDirs())
		{
			const int3 neighbour = current + direction;
			if(aiNk->cc->isInTheMap(neighbour) && !visited.contains(neighbour))
			{
				visited.insert(neighbour);
				pending.push(neighbour);
			}
		}
	}

	if(bestTarget.value <= 0)
		return std::nullopt;
	return bestTarget;
}

std::set<int3> getVisibleWaterComponent(const Nullkiller * aiNk, const int3 & start)
{
	const auto * teamState = aiNk->cc->getPlayerTeam(aiNk->playerID);
	const auto & fow = teamState->fogOfWarMap;
	std::queue<int3> pending;
	std::set<int3> scheduled;
	std::set<int3> result;
	pending.push(start);
	scheduled.insert(start);

	while(!pending.empty())
	{
		const int3 current = pending.front();
		pending.pop();
		if(!aiNk->cc->isInTheMap(current) || !fow[current])
			continue;

		const auto * tile = aiNk->cc->getTile(current, false);
		if(!tile || !tile->isWater())
			continue;

		result.insert(current);
		for(const int3 & direction : int3::getDirs())
		{
			const int3 neighbour = current + direction;
			if(aiNk->cc->isInTheMap(neighbour) && !scheduled.contains(neighbour))
			{
				scheduled.insert(neighbour);
				pending.push(neighbour);
			}
		}
	}

	return result;
}

std::optional<WaterExplorationTarget> getVirtualBoatExplorationTarget(
	const Nullkiller * aiNk,
	const IShipyard * shipyard)
{
	if(!shipyard)
		return std::nullopt;

	const auto status = shipyard->shipyardStatus();
	const int3 boatLocation = shipyard->bestLocation();
	if(status != IBoatGenerator::GOOD || !boatLocation.isValid())
		return std::nullopt;

	WaterExplorationTarget bestTarget;
	for(const CGHeroInstance * hero : aiNk->cc->getHeroesInfo())
	{
		const auto target = findWaterExplorationTarget(aiNk, boatLocation, hero->getSightRadius());
		if(target && target->value > bestTarget.value)
			bestTarget = *target;
	}
	if(bestTarget.value <= 0)
		return std::nullopt;

	BoatExplorationCandidate candidate;
	candidate.available = true;
	candidate.hiddenTilesDiscovered = bestTarget.value;
	const auto evaluation = evaluateBoatExplorationCandidate(candidate);
	if(!evaluation.accepted)
		return std::nullopt;

	bestTarget.value = evaluation.explorationValue;
	return bestTarget;
}

bool hasUsableWaterTransport(
	const Nullkiller * aiNk,
	const WaterExplorationTarget & explorationTarget)
{
	const auto waterComponent = getVisibleWaterComponent(aiNk, explorationTarget.tile);
	for(const CGHeroInstance * hero : aiNk->cc->getHeroesInfo())
	{
		if(hero->inBoat() && waterComponent.contains(hero->visitablePos()))
			return true;
	}

	for(const ObjectInstanceID objectId : aiNk->memory->visitableObjs)
	{
		const auto * boat = aiNk->cc->getObj(objectId, false);
		if(!boat || boat->ID != Obj::BOAT || !waterComponent.contains(boat->visitablePos()))
			continue;

		const auto * topObject = aiNk->cc->getTopObj(boat->visitablePos());
		if(!topObject || topObject->id != boat->id)
			continue;

		const auto paths = aiNk->pathfinder->getPathInfo(
			boat->visitablePos(),
			aiNk->isObjectGraphAllowed());
		const bool hasBoardingRoute = std::ranges::any_of(paths, [aiNk](const AIPath & path)
		{
			return path.targetHero->getOwner() == aiNk->playerID && !path.targetHero->inBoat();
		});
		if(hasBoardingRoute)
			return true;
	}

	return false;
}

void addWaterExplorationTasks(Goals::TGoalVec & tasks, const Nullkiller * aiNk)
{
	struct Transport
	{
		int3 position;
		const CGHeroInstance * sailingHero = nullptr;
	};

	std::vector<Transport> transports;
	const auto heroes = aiNk->cc->getHeroesInfo();
	for(const CGHeroInstance * hero : heroes)
	{
		if(hero->inBoat())
			transports.push_back({hero->visitablePos(), hero});
	}

	for(const ObjectInstanceID objectId : aiNk->memory->visitableObjs)
	{
		const auto * boat = aiNk->cc->getObj(objectId, false);
		if(!boat || boat->ID != Obj::BOAT)
			continue;

		const auto * topObject = aiNk->cc->getTopObj(boat->visitablePos());
		if(topObject && topObject->id == boat->id)
			transports.push_back({boat->visitablePos(), nullptr});
	}

	std::set<std::pair<ObjectInstanceID, int3>> addedRoutes;
	for(const Transport & transport : transports)
	{
		for(const CGHeroInstance * hero : heroes)
		{
			if((transport.sailingHero && transport.sailingHero != hero)
				|| (!transport.sailingHero && hero->inBoat()))
			{
				continue;
			}

			const auto explorationTarget = findWaterExplorationTarget(
				aiNk,
				transport.position,
				hero->getSightRadius());
			if(!explorationTarget || explorationTarget->tile == hero->visitablePos())
				continue;

			const auto route = std::make_pair(hero->id, explorationTarget->tile);
			if(addedRoutes.contains(route))
				continue;

			auto paths = aiNk->pathfinder->getPathInfo(
				explorationTarget->tile,
				aiNk->isObjectGraphAllowed());
			std::erase_if(paths, [hero](const AIPath & path)
			{
				return path.targetHero != hero;
			});
			const auto visitGoals = CaptureObjectsBehavior::getVisitGoals(paths, aiNk, nullptr, true);
			bool added = false;
			for(const auto & visitGoal : visitGoals)
			{
				if(visitGoal->invalid())
					continue;

				tasks.push_back(sptr(Composition()
					.addNext(ExplorationPoint(explorationTarget->tile, explorationTarget->value))
					.addNext(visitGoal)));
				added = true;
			}
			if(added)
				addedRoutes.insert(route);
		}
	}
}

void addVirtualBoatExplorationTasks(Goals::TGoalVec & tasks, const Nullkiller * aiNk)
{
	std::map<ObjectInstanceID, const IShipyard *> shipyards;
	for(const CGTownInstance * town : aiNk->cc->getTownsInfo())
		shipyards[town->id] = town;

	for(const CGObjectInstance * object : aiNk->cc->getMyObjects())
	{
		const auto * shipyard = dynamic_cast<const IShipyard *>(object);
		if(shipyard)
			shipyards[object->id] = shipyard;
	}

	for(const ObjectInstanceID objectId : aiNk->memory->visitableObjs)
	{
		const auto * object = aiNk->cc->getObj(objectId, false);
		const auto * shipyard = dynamic_cast<const IShipyard *>(object);
		if(shipyard)
			shipyards[objectId] = shipyard;
	}

	for(const auto & [objectId, shipyard] : shipyards)
	{
		const auto explorationTarget = getVirtualBoatExplorationTarget(aiNk, shipyard);
		if(!explorationTarget)
			continue;
		if(hasUsableWaterTransport(aiNk, *explorationTarget))
			continue;

		const CGObjectInstance * shipyardObject = aiNk->cc->getObj(objectId, false);
		if(!shipyardObject)
			continue;

		TResources boatCost;
		shipyard->getBoatCost(boatCost);
		const bool canAfford = aiNk->getFreeResources().canAfford(boatCost);
		if(!canAfford && aiNk->getLockedResources().canAfford(boatCost))
			continue;

		ExplorationPoint explorationPoint(explorationTarget->tile, explorationTarget->value);
		explorationPoint.setobjid(objectId.getNum());
		const int3 shipyardPosition = shipyardObject->visitablePos();
		const auto heroes = aiNk->cc->getHeroesInfo();
		const auto heroAtShipyard = std::ranges::find_if(
			heroes,
			[shipyardPosition](const CGHeroInstance * hero)
			{
				return !hero->inBoat() && hero->visitablePos() == shipyardPosition;
			});
		const bool canBuildHere = aiNk->cc->getPlayerRelations(
			aiNk->playerID,
			shipyardObject->getOwner()) != PlayerRelations::ENEMIES;
		if(heroAtShipyard != heroes.end() && canBuildHere)
		{
			Composition construction;
			construction.addNext(explorationPoint);
			if(canAfford)
				construction.addNext(BuildBoat(shipyard));
			else
				construction.addNext(SaveResources(boatCost));
			tasks.push_back(sptr(construction));
			continue;
		}

		auto paths = aiNk->pathfinder->getPathInfo(
			shipyardPosition,
			aiNk->isObjectGraphAllowed());
		std::erase_if(paths, [shipyardPosition](const AIPath & path)
		{
			return path.targetHero->inBoat()
				|| path.targetHero->visitablePos() == shipyardPosition;
		});
		const auto visitGoals = CaptureObjectsBehavior::getVisitGoals(paths, aiNk, shipyardObject, true);
		for(const auto & visitGoal : visitGoals)
		{
			if(visitGoal->invalid())
				continue;

			Composition exploration;
			exploration.addNext(explorationPoint);
			if(canAfford)
			{
				// Visiting an unowned shipyard transfers ownership synchronously. Build in the
				// same execution sequence so another exploration pass cannot move the hero away.
				exploration.addNextSequence({visitGoal, sptr(BuildBoat(shipyard))});
			}
			else
				exploration.addNext(SaveResources(boatCost));
			tasks.push_back(sptr(exploration));
		}
	}
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

uint64_t getPortalHeroStrength(const CGHeroInstance * hero, const CCreatureSet * army)
{
	return static_cast<uint64_t>(getNormalizedHeroStrength(hero) * army->getArmyStrength());
}

bool isBetterPortalReinforcementPath(const AIPath & candidate, const AIPath & current)
{
	const uint64_t candidateStrength = getPortalHeroStrength(candidate.targetHero, candidate.heroArmy);
	const uint64_t currentStrength = getPortalHeroStrength(current.targetHero, current.heroArmy);
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

		const uint64_t strength = getPortalHeroStrength(hero, hero);
		if(!result || strength > *result)
			result = strength;
	}
	return result;
}

bool hasEnemyObjectiveIsolatedBehindPortal(
	const CGObjectInstance * entrance,
	const Nullkiller * aiNk)
{
	const auto unreturnedHeroes = aiNk->memory->getUnreturnedOneWayPortalHeroes(entrance->id);
	for(const ObjectInstanceID objectId : aiNk->memory->visitableObjs)
	{
		const auto * object = aiNk->cc->getObj(objectId, false);
		if(!object
			|| (object->ID != Obj::TOWN && object->ID != Obj::HERO)
			|| !object->getOwner().isValidPlayer()
			|| aiNk->cc->getPlayerRelations(aiNk->playerID, object->getOwner()) != PlayerRelations::ENEMIES
			|| !aiNk->cc->isVisible(object->visitablePos()))
		{
			continue;
		}

		const auto paths = aiNk->pathfinder->getPathInfo(
			object->visitablePos(),
			aiNk->isObjectGraphAllowed());
		const bool reachableWithoutExpedition = vstd::contains_if(paths, [&](const AIPath & path)
		{
			return path.targetHero
				&& path.targetHero->getOwner() == aiNk->playerID
				&& !vstd::contains(unreturnedHeroes, path.targetHero->id)
				&& !path.getFirstBlockedAction();
		});
		if(!reachableWithoutExpedition)
			return true;
	}
	return false;
}

std::optional<AIPath> findBestPortalPath(
	const std::vector<AIPath> & paths,
	const Nullkiller * aiNk,
	const CGObjectInstance * entrance,
	std::optional<HeroRole> requiredRole,
	std::optional<ObjectInstanceID> requiredHero,
	std::optional<std::pair<uint64_t, uint64_t>> guardFailure = std::nullopt,
	std::optional<uint64_t> minimumStrength = std::nullopt,
	bool preferStrongest = false)
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

		const uint64_t heroStrength = getPortalHeroStrength(hero, path.heroArmy);
		if(minimumStrength && heroStrength <= *minimumStrength)
		{
			logAi->trace(
				"One-way portal reinforcement candidate %s rejected: strength %lld does not exceed expedition strength %lld",
				hero->getNameTextID(),
				static_cast<long long>(heroStrength),
				static_cast<long long>(*minimumStrength));
			continue;
		}

		if(guardFailure)
		{
			if(heroStrength <= guardFailure->second)
			{
				logAi->trace(
					"One-way portal retry candidate %s rejected: strength %lld does not exceed failed hero strength %lld",
					hero->getNameTextID(),
					static_cast<long long>(heroStrength),
					static_cast<long long>(guardFailure->second));
				continue;
			}

			if(!isSafeToVisit(
				hero,
				path.heroArmy,
				guardFailure->first,
				aiNk->settings->getSafeAttackRatio()))
			{
				logAi->trace(
					"One-way portal retry candidate %s rejected: remembered exit guard danger %lld is unsafe",
					hero->getNameTextID(),
					static_cast<long long>(guardFailure->first));
				continue;
			}
		}

		if(!result
			|| (preferStrongest
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

std::optional<AIPath> findPortalProbePath(
	const CGObjectInstance * entrance,
	const Nullkiller * aiNk,
	std::optional<std::pair<uint64_t, uint64_t>> guardFailure,
	std::optional<uint64_t> minimumStrength)
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
				reservedHero->getNameTextID());
			return std::nullopt;
		}

		if(auto reservedPath = findBestPortalPath(
			paths,
			aiNk,
			entrance,
			std::nullopt,
			reservation,
			guardFailure,
			minimumStrength,
			minimumStrength.has_value()))
		{
			return reservedPath;
		}

		logAi->debug(
			"Clearing stale one-way portal %d reservation for hero %d",
			entrance->id.getNum(),
			reservation->getNum());
		aiNk->memory->clearOneWayPortalReservation(entrance->id);
	}

	if(!guardFailure && !minimumStrength)
	{
		if(auto scoutPath = findBestPortalPath(paths, aiNk, entrance, HeroRole::SCOUT, std::nullopt))
			return scoutPath;
	}

	return findBestPortalPath(
		paths,
		aiNk,
		entrance,
		HeroRole::MAIN,
		std::nullopt,
		guardFailure,
		minimumStrength,
		minimumStrength.has_value());
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

		if(aiNk->memory->hasActiveOneWayPortalJourney(entrance->id)
			&& !desperation)
		{
			logAi->debug(
				"Suppressing repeated traffic through one-way portal %d: an expedition is still active",
				entrance->id.getNum());
			continue;
		}

		const auto guardFailure = aiNk->memory->getOneWayPortalGuardFailure(entrance->id);
		std::optional<uint64_t> reinforcementStrength;

		if(aiNk->memory->wasOneWayPortalProbed(entrance->id)
			&& !aiNk->memory->hasKnownOneWayPortalReturn(entrance->id)
			&& !guardFailure
			&& !desperation)
		{
			reinforcementStrength = getStrongestUnreturnedPortalHeroStrength(entrance, aiNk);
			if(!reinforcementStrength || !hasEnemyObjectiveIsolatedBehindPortal(entrance, aiNk))
			{
				logAi->debug(
					"Suppressing repeated traffic through one-way portal %d: no return route is known",
					entrance->id.getNum());
				continue;
			}
		}

		const auto path = findPortalProbePath(entrance, aiNk, guardFailure, reinforcementStrength);
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
			desperationProbe ? "desperation" : reinforcementStrength ? "reinforce" : "probe",
			entrance->id.getNum(),
			teleport ? teleport->channel.getNum() : -1,
			aiNk->heroManager->getHeroRoleOrDefaultInefficient(path->targetHero) == HeroRole::SCOUT ? "scout" : "main",
			path->targetHero->getNameTextID());
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

	addVirtualBoatExplorationTasks(tasks, aiNk);
	addWaterExplorationTasks(tasks, aiNk);

	for(const ObjectInstanceID objId : aiNk->memory->visitableObjs)
	{
		const CGObjectInstance * obj = aiNk->cc->getObjInstance(objId);
		if(!obj)
			continue;

		switch(obj->ID.num)
		{
			case Obj::BOAT:
				break;
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
