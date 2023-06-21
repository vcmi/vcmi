/*
 * CPathfinder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CPathfinder.h"

#include "INodeStorage.h"
#include "PathfinderOptions.h"
#include "PathfindingRules.h"
#include "TurnInfo.h"

#include "../CGameState.h"
#include "../CPlayerState.h"
#include "../TerrainHandler.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapping/CMap.h"

VCMI_LIB_NAMESPACE_BEGIN

std::vector<int3> CPathfinderHelper::getNeighbourTiles(const PathNodeInfo & source) const
{
	std::vector<int3> neighbourTiles;
	neighbourTiles.reserve(8);

	getNeighbours(
		*source.tile,
		source.node->coord,
		neighbourTiles,
		boost::logic::indeterminate,
		source.node->layer == EPathfindingLayer::SAIL);

	if(source.isNodeObjectVisitable())
	{
		vstd::erase_if(neighbourTiles, [&](const int3 & tile) -> bool 
		{
			return !canMoveBetween(tile, source.nodeObject->visitablePos());
		});
	}

	return neighbourTiles;
}

CPathfinder::CPathfinder(CGameState * _gs, std::shared_ptr<PathfinderConfig> config): 
	gamestate(_gs),
	config(std::move(config))
{
	initializeGraph();
}


void CPathfinder::push(CGPathNode * node)
{
	if(node && !node->inPQ)
	{
		node->inPQ = true;
		node->pq = &this->pq;
		auto handle = pq.push(node);
		node->pqHandle = handle;
	}
}

CGPathNode * CPathfinder::topAndPop()
{
	auto * node = pq.top();

	pq.pop();
	node->inPQ = false;
	node->pq = nullptr;
	return node;
}

void CPathfinder::calculatePaths()
{
	//logGlobal->info("Calculating paths for hero %s (adress  %d) of player %d", hero->name, hero , hero->tempOwner);

	//initial tile - set cost on 0 and add to the queue
	std::vector<CGPathNode *> initialNodes = config->nodeStorage->getInitialNodes();
	int counter = 0;

	for(auto * initialNode : initialNodes)
	{
		if(!gamestate->isInTheMap(initialNode->coord)/* || !gs->map->isInTheMap(dest)*/) //check input
		{
			logGlobal->error("CGameState::calculatePaths: Hero outside the gs->map? How dare you...");
			throw std::runtime_error("Wrong checksum");
		}

		source.setNode(gamestate, initialNode);
		auto * hlp = config->getOrCreatePathfinderHelper(source, gamestate);

		if(hlp->isHeroPatrolLocked())
			continue;

		pq.push(initialNode);
	}

	while(!pq.empty())
	{
		counter++;
		auto * node = topAndPop();

		source.setNode(gamestate, node);
		source.node->locked = true;

		int movement = source.node->moveRemains;
		uint8_t turn = source.node->turns;
		float cost = source.node->getCost();

		auto * hlp = config->getOrCreatePathfinderHelper(source, gamestate);

		hlp->updateTurnInfo(turn);
		if(!movement)
		{
			hlp->updateTurnInfo(++turn);
			movement = hlp->getMaxMovePoints(source.node->layer);
			if(!hlp->passOneTurnLimitCheck(source))
				continue;
		}

		source.isInitialPosition = source.nodeHero == hlp->hero;
		source.updateInfo(hlp, gamestate);

		//add accessible neighbouring nodes to the queue
		auto neighbourNodes = config->nodeStorage->calculateNeighbours(source, config.get(), hlp);
		for(CGPathNode * neighbour : neighbourNodes)
		{
			if(neighbour->locked)
				continue;

			if(!hlp->isLayerAvailable(neighbour->layer))
				continue;

			destination.setNode(gamestate, neighbour);
			hlp = config->getOrCreatePathfinderHelper(destination, gamestate);

			if(!hlp->isPatrolMovementAllowed(neighbour->coord))
				continue;

			/// Check transition without tile accessability rules
			if(source.node->layer != neighbour->layer && !isLayerTransitionPossible())
				continue;

			destination.turn = turn;
			destination.movementLeft = movement;
			destination.cost = cost;
			destination.updateInfo(hlp, gamestate);
			destination.isGuardianTile = destination.guarded && isDestinationGuardian();

			for(const auto & rule : config->rules)
			{
				rule->process(source, destination, config.get(), hlp);

				if(destination.blocked)
					break;
			}

			if(!destination.blocked)
				push(destination.node);

		} //neighbours loop

		//just add all passable teleport exits
		hlp = config->getOrCreatePathfinderHelper(source, gamestate);

		/// For now we disable teleports usage for patrol movement
		/// VCAI not aware about patrol and may stuck while attempt to use teleport
		if(hlp->patrolState == CPathfinderHelper::PATROL_RADIUS)
			continue;

		auto teleportationNodes = config->nodeStorage->calculateTeleportations(source, config.get(), hlp);
		for(CGPathNode * teleportNode : teleportationNodes)
		{
			if(teleportNode->locked)
				continue;
			/// TODO: We may consider use invisible exits on FoW border in future
			/// Useful for AI when at least one tile around exit is visible and passable
			/// Objects are usually visible on FoW border anyway so it's not cheating.
			///
			/// For now it's disabled as it's will cause crashes in movement code.
			if(teleportNode->accessible == EPathAccessibility::BLOCKED)
				continue;

			destination.setNode(gamestate, teleportNode);
			destination.turn = turn;
			destination.movementLeft = movement;
			destination.cost = cost;

			if(destination.isBetterWay())
			{
				destination.action = getTeleportDestAction();
				config->nodeStorage->commit(destination, source);

				if(destination.node->action == EPathNodeAction::TELEPORT_NORMAL)
					push(destination.node);
			}
		}
	} //queue loop

	logAi->trace("CPathfinder finished with %s iterations", std::to_string(counter));
}

std::vector<int3> CPathfinderHelper::getAllowedTeleportChannelExits(const TeleportChannelID & channelID) const
{
	std::vector<int3> allowedExits;

	for(const auto & objId : getTeleportChannelExits(channelID, hero->tempOwner))
	{
		const auto * obj = getObj(objId);
		if(dynamic_cast<const CGWhirlpool *>(obj))
		{
			auto pos = obj->getBlockedPos();
			for(const auto & p : pos)
			{
				if(gs->map->getTile(p).topVisitableId() == obj->ID)
					allowedExits.push_back(p);
			}
		}
		else if(obj && CGTeleport::isExitPassable(gs, hero, obj))
			allowedExits.push_back(obj->visitablePos());
	}

	return allowedExits;
}

std::vector<int3> CPathfinderHelper::getCastleGates(const PathNodeInfo & source) const
{
	std::vector<int3> allowedExits;

	auto towns = getPlayerState(hero->tempOwner)->towns;
	for(const auto & town : towns)
	{
		if(town->id != source.nodeObject->id && town->visitingHero == nullptr
			&& town->hasBuilt(BuildingID::CASTLE_GATE, ETownType::INFERNO))
		{
			allowedExits.push_back(town->visitablePos());
		}
	}

	return allowedExits;
}

std::vector<int3> CPathfinderHelper::getTeleportExits(const PathNodeInfo & source) const
{
	std::vector<int3> teleportationExits;

	const auto * objTeleport = dynamic_cast<const CGTeleport *>(source.nodeObject);
	if(isAllowedTeleportEntrance(objTeleport))
	{
		for(const auto & exit : getAllowedTeleportChannelExits(objTeleport->channel))
		{
			teleportationExits.push_back(exit);
		}
	}
	else if(options.useCastleGate
		&& (source.nodeObject->ID == Obj::TOWN && source.nodeObject->subID == ETownType::INFERNO
		&& source.objectRelations != PlayerRelations::ENEMIES))
	{
		/// TODO: Find way to reuse CPlayerSpecificInfoCallback::getTownsInfo
		/// This may be handy if we allow to use teleportation to friendly towns
		for(const auto & exit : getCastleGates(source))
		{
			teleportationExits.push_back(exit);
		}
	}

	return teleportationExits;
}

bool CPathfinderHelper::isHeroPatrolLocked() const
{
	return patrolState == PATROL_LOCKED;
}

bool CPathfinderHelper::isPatrolMovementAllowed(const int3 & dst) const
{
	if(patrolState == PATROL_RADIUS)
	{
		if(!vstd::contains(patrolTiles, dst))
			return false;
	}

	return true;
}

bool CPathfinder::isLayerTransitionPossible() const
{
	ELayer destLayer = destination.node->layer;

	/// No layer transition allowed when previous node action is BATTLE
	if(source.node->action == EPathNodeAction::BATTLE)
		return false;

	switch(source.node->layer)
	{
	case ELayer::LAND:
		if(destLayer == ELayer::AIR)
		{
			if(!config->options.lightweightFlyingMode || source.isInitialPosition)
				return true;
		}
		else if(destLayer == ELayer::SAIL)
		{
			if(destination.tile->isWater())
				return true;
		}
		else
			return true;

		break;

	case ELayer::SAIL:
		if(destLayer == ELayer::LAND && !destination.tile->isWater())
			return true;

		break;

	case ELayer::AIR:
		if(destLayer == ELayer::LAND)
			return true;

		break;

	case ELayer::WATER:
		if(destLayer == ELayer::LAND)
			return true;

		break;
	}

	return false;
}

EPathNodeAction CPathfinder::getTeleportDestAction() const
{
	EPathNodeAction action = EPathNodeAction::TELEPORT_NORMAL;

	if(destination.isNodeObjectVisitable() && destination.nodeHero)
	{
		if(destination.heroRelations == PlayerRelations::ENEMIES)
			action = EPathNodeAction::TELEPORT_BATTLE;
		else
			action = EPathNodeAction::TELEPORT_BLOCKING_VISIT;
	}

	return action;
}

bool CPathfinder::isDestinationGuardian() const
{
	return gamestate->guardingCreaturePosition(destination.node->coord) == destination.node->coord;
}

void CPathfinderHelper::initializePatrol()
{
	auto state = PATROL_NONE;

	if(hero->patrol.patrolling && !getPlayerState(hero->tempOwner)->human)
	{
		if(hero->patrol.patrolRadius)
		{
			state = PATROL_RADIUS;
			gs->getTilesInRange(patrolTiles, hero->patrol.initialPos, hero->patrol.patrolRadius, std::optional<PlayerColor>(), 0, int3::DIST_MANHATTAN);
		}
		else
			state = PATROL_LOCKED;
	}

	patrolState = state;
}

void CPathfinder::initializeGraph()
{
	INodeStorage * nodeStorage = config->nodeStorage.get();
	nodeStorage->initialize(config->options, gamestate);
}

bool CPathfinderHelper::canMoveBetween(const int3 & a, const int3 & b) const
{
	return gs->checkForVisitableDir(a, b);
}

bool CPathfinderHelper::isAllowedTeleportEntrance(const CGTeleport * obj) const
{
	if(!obj || !isTeleportEntrancePassable(obj, hero->tempOwner))
		return false;

	const auto * whirlpool = dynamic_cast<const CGWhirlpool *>(obj);
	if(whirlpool)
	{
		if(addTeleportWhirlpool(whirlpool))
			return true;
	}
	else if(addTeleportTwoWay(obj) || addTeleportOneWay(obj) || addTeleportOneWayRandom(obj))
		return true;

	return false;
}

bool CPathfinderHelper::addTeleportTwoWay(const CGTeleport * obj) const
{
	return options.useTeleportTwoWay && isTeleportChannelBidirectional(obj->channel, hero->tempOwner);
}

bool CPathfinderHelper::addTeleportOneWay(const CGTeleport * obj) const
{
	if(options.useTeleportOneWay && isTeleportChannelUnidirectional(obj->channel, hero->tempOwner))
	{
		auto passableExits = CGTeleport::getPassableExits(gs, hero, getTeleportChannelExits(obj->channel, hero->tempOwner));
		if(passableExits.size() == 1)
			return true;
	}
	return false;
}

bool CPathfinderHelper::addTeleportOneWayRandom(const CGTeleport * obj) const
{
	if(options.useTeleportOneWayRandom && isTeleportChannelUnidirectional(obj->channel, hero->tempOwner))
	{
		auto passableExits = CGTeleport::getPassableExits(gs, hero, getTeleportChannelExits(obj->channel, hero->tempOwner));
		if(passableExits.size() > 1)
			return true;
	}
	return false;
}

bool CPathfinderHelper::addTeleportWhirlpool(const CGWhirlpool * obj) const
{
	return options.useTeleportWhirlpool && hasBonusOfType(BonusType::WHIRLPOOL_PROTECTION) && obj;
}

int CPathfinderHelper::movementPointsAfterEmbark(int movement, int basicCost, bool disembark) const
{
	return hero->movementPointsAfterEmbark(movement, basicCost, disembark, getTurnInfo());
}

bool CPathfinderHelper::passOneTurnLimitCheck(const PathNodeInfo & source) const
{

	if(!options.oneTurnSpecialLayersLimit)
		return true;

	if(source.node->layer == EPathfindingLayer::WATER)
		return false;
	if(source.node->layer == EPathfindingLayer::AIR)
	{
		return options.originalMovementRules && source.node->accessible == EPathAccessibility::ACCESSIBLE;
	}

	return true;
}

CPathfinderHelper::CPathfinderHelper(CGameState * gs, const CGHeroInstance * Hero, const PathfinderOptions & Options):
	CGameInfoCallback(gs, std::optional<PlayerColor>()),
	turn(-1),
	hero(Hero),
	options(Options),
	owner(Hero->tempOwner)
{
	turnsInfo.reserve(16);
	updateTurnInfo();
	initializePatrol();
}

CPathfinderHelper::~CPathfinderHelper()
{
	for(auto * ti : turnsInfo)
		delete ti;
}

void CPathfinderHelper::updateTurnInfo(const int Turn)
{
	if(turn != Turn)
	{
		turn = Turn;
		if(turn >= turnsInfo.size())
		{
			auto * ti = new TurnInfo(hero, turn);
			turnsInfo.push_back(ti);
		}
	}
}

bool CPathfinderHelper::isLayerAvailable(const EPathfindingLayer & layer) const
{
	switch(layer)
	{
	case EPathfindingLayer::AIR:
		if(!options.useFlying)
			return false;

		break;

	case EPathfindingLayer::WATER:
		if(!options.useWaterWalking)
			return false;

		break;
	}

	return turnsInfo[turn]->isLayerAvailable(layer);
}

const TurnInfo * CPathfinderHelper::getTurnInfo() const
{
	return turnsInfo[turn];
}

bool CPathfinderHelper::hasBonusOfType(const BonusType type, const int subtype) const
{
	return turnsInfo[turn]->hasBonusOfType(type, subtype);
}

int CPathfinderHelper::getMaxMovePoints(const EPathfindingLayer & layer) const
{
	return turnsInfo[turn]->getMaxMovePoints(layer);
}

void CPathfinderHelper::getNeighbours(
	const TerrainTile & srcTile,
	const int3 & srcCoord,
	std::vector<int3> & vec,
	const boost::logic::tribool & onLand,
	const bool limitCoastSailing) const
{
	CMap * map = gs->map;

	static const int3 dirs[] = {
		int3(-1, +1, +0),	int3(0, +1, +0),	int3(+1, +1, +0),
		int3(-1, +0, +0),	/* source pos */	int3(+1, +0, +0),
		int3(-1, -1, +0),	int3(0, -1, +0),	int3(+1, -1, +0)
	};

	for(const auto & dir : dirs)
	{
		const int3 destCoord = srcCoord + dir;
		if(!map->isInTheMap(destCoord))
			continue;

		const TerrainTile & destTile = map->getTile(destCoord);
		if(!destTile.terType->isPassable())
			continue;

// 		//we cannot visit things from blocked tiles
// 		if(srcTile.blocked && !srcTile.visitable && destTile.visitable && srcTile.blockingObjects.front()->ID != HEROI_TYPE)
// 		{
// 			continue;
// 		}

		/// Following condition let us avoid diagonal movement over coast when sailing
		if(srcTile.terType->isWater() && limitCoastSailing && destTile.terType->isWater() && dir.x && dir.y) //diagonal move through water
		{
			const int3 horizontalNeighbour = srcCoord + int3{dir.x, 0, 0};
			const int3 verticalNeighbour = srcCoord + int3{0, dir.y, 0};
			if(map->getTile(horizontalNeighbour).terType->isLand() || map->getTile(verticalNeighbour).terType->isLand())
				continue;
		}

		if(indeterminate(onLand) || onLand == destTile.terType->isLand())
		{
			vec.push_back(destCoord);
		}
	}
}

int CPathfinderHelper::getMovementCost(
	const PathNodeInfo & src,
	const PathNodeInfo & dst,
	const int remainingMovePoints,
	const bool checkLast) const
{
	return getMovementCost(
		src.coord,
		dst.coord,
		src.tile,
		dst.tile,
		remainingMovePoints,
		checkLast,
		dst.node->layer == EPathfindingLayer::SAIL,
		dst.node->layer == EPathfindingLayer::WATER
	);
}

int CPathfinderHelper::getMovementCost(
	const int3 & src,
	const int3 & dst,
	const TerrainTile * ct,
	const TerrainTile * dt,
	const int remainingMovePoints,
	const bool checkLast,
	boost::logic::tribool isDstSailLayer,
	boost::logic::tribool isDstWaterLayer) const
{
	if(src == dst) //same tile
		return 0;

	const auto * ti = getTurnInfo();

	if(ct == nullptr || dt == nullptr)
	{
		ct = hero->cb->getTile(src);
		dt = hero->cb->getTile(dst);
	}

	bool isSailLayer;
	if(indeterminate(isDstSailLayer))
		isSailLayer = hero->boat && hero->boat->layer == EPathfindingLayer::SAIL && dt->terType->isWater();
	else
		isSailLayer = static_cast<bool>(isDstSailLayer);

	bool isWaterLayer;
	if(indeterminate(isDstWaterLayer))
		isWaterLayer = ((hero->boat && hero->boat->layer == EPathfindingLayer::WATER) || ti->hasBonusOfType(BonusType::WATER_WALKING)) && dt->terType->isWater();
	else
		isWaterLayer = static_cast<bool>(isDstWaterLayer);
	
	bool isAirLayer = (hero->boat && hero->boat->layer == EPathfindingLayer::AIR) || ti->hasBonusOfType(BonusType::FLYING_MOVEMENT);

	int ret = hero->getTileCost(*dt, *ct, ti);
	if(isSailLayer)
	{
		if(ct->hasFavorableWinds())
			ret = static_cast<int>(ret * 2.0 / 3);
	}
	else if(isAirLayer)
		vstd::amin(ret, GameConstants::BASE_MOVEMENT_COST + ti->valOfBonuses(BonusType::FLYING_MOVEMENT));
	else if(isWaterLayer && ti->hasBonusOfType(BonusType::WATER_WALKING))
		ret = static_cast<int>(ret * (100.0 + ti->valOfBonuses(BonusType::WATER_WALKING)) / 100.0);

	if(src.x != dst.x && src.y != dst.y) //it's diagonal move
	{
		int old = ret;
		ret = static_cast<int>(ret * M_SQRT2);
		//diagonal move costs too much but normal move is possible - allow diagonal move for remaining move points
		// https://heroes.thelazy.net/index.php/Movement#Diagonal_move_exception
		if(ret > remainingMovePoints && remainingMovePoints >= old)
		{
			return remainingMovePoints;
		}
	}

	const int left = remainingMovePoints - ret;
	constexpr auto maxCostOfOneStep = static_cast<int>(175 * M_SQRT2); // diagonal move on Swamp - 247 MP
	if(checkLast && left > 0 && left <= maxCostOfOneStep) //it might be the last tile - if no further move possible we take all move points
	{
		std::vector<int3> vec;
		vec.reserve(8); //optimization
		getNeighbours(*dt, dst, vec, ct->terType->isLand(), true);
		for(const auto & elem : vec)
		{
			int fcost = getMovementCost(dst, elem, nullptr, nullptr, left, false);
			if(fcost <= left)
			{
				return ret;
			}
		}
		ret = remainingMovePoints;
	}

	return ret;
}

VCMI_LIB_NAMESPACE_END
