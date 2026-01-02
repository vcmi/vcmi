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

#include "../IGameSettings.h"
#include "../CPlayerState.h"
#include "../TerrainHandler.h"
#include "../RoadHandler.h"
#include "../callback/IGameInfoCallback.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapping/CMap.h"
#include "../spells/CSpellHandler.h"
#include "spells/ISpellMechanics.h"

VCMI_LIB_NAMESPACE_BEGIN

bool CPathfinderHelper::canMoveFromNode(const PathNodeInfo & source) const
{
	// we can always make the first step, even when standing on object
	if(source.node->theNodeBefore == nullptr)
		return true;

	if (!source.nodeObject)
		return true;

	if (!source.isNodeObjectVisitable())
		return true;

	// we can always move from visitable object if hero has teleported here (e.g. went through monolith)
	if (source.node->isTeleportAction())
		return true;

	// we can not go through teleporters since moving onto a teleport will teleport hero and may invalidate path (e.g. one-way teleport or enemy hero on other side)
	if (dynamic_cast<const CGTeleport*>(source.nodeObject) != nullptr)
		return false;

	return true;
}

void CPathfinderHelper::calculateNeighbourTiles(NeighbourTilesVector & result, const PathNodeInfo & source) const
{
	result.clear();

	if (!canMoveFromNode(source))
		return;

	getNeighbours(
		*source.tile,
		source.node->coord,
		result,
		boost::logic::indeterminate,
		source.node->layer == EPathfindingLayer::SAIL);

	if(source.isNodeObjectVisitable())
	{
		vstd::erase_if(result, [&](const int3 & tile) -> bool
		{
			return !canMoveBetween(tile, source.nodeObject->visitablePos());
		});
	}
}

CPathfinder::CPathfinder(const IGameInfoCallback & gameInfo, std::shared_ptr<PathfinderConfig> config):
	gameInfo(gameInfo),
	config(std::move(config))
{
	initializeGraph();
}


void CPathfinder::push(CGPathNode * node)
{
	if(node && !node->inPQ())
	{
		node->pq = &this->pq;
		auto handle = pq.push(node);
		node->pqHandle = handle;
	}
}

CGPathNode * CPathfinder::topAndPop()
{
	auto * node = pq.top();

	pq.pop();
	node->pq = nullptr;
	return node;
}

void CPathfinder::calculatePaths()
{
	//logGlobal->info("Calculating paths for hero %s (address  %d) of player %d", hero->name, hero , hero->tempOwner);

	//initial tile - set cost on 0 and add to the queue
	std::vector<CGPathNode *> initialNodes = config->nodeStorage->getInitialNodes();
	int counter = 0;

	for(auto * initialNode : initialNodes)
	{
		if(!gameInfo.isInTheMap(initialNode->coord)/* || !gameInfo.getMap().isInTheMap(dest)*/) //check input
		{
			logGlobal->error("CgameInfo::calculatePaths: Hero outside the gameInfo.map? How dare you...");
			throw std::runtime_error("Wrong checksum");
		}

		source.setNode(gameInfo, initialNode);
		auto * hlp = config->getOrCreatePathfinderHelper(source, gameInfo);
		if(hlp->isHeroPatrolLocked())
			continue;

		pq.push(initialNode);
	}

	std::vector<CGPathNode *> neighbourNodes;

	while(!pq.empty())
	{
		counter++;
		auto * node = topAndPop();

		source.setNode(gameInfo, node);
		source.node->locked = true;

		int movement = source.node->moveRemains;
		uint8_t turn = source.node->turns;
		float cost = source.node->getCost();

		auto * hlp = config->getOrCreatePathfinderHelper(source, gameInfo);
		hlp->updateTurnInfo(turn);
		if(movement == 0)
		{
			hlp->updateTurnInfo(++turn);
			movement = hlp->getMaxMovePoints(source.node->layer);
			if(!hlp->passOneTurnLimitCheck(source))
				continue;
			if(turn > hlp->options.turnLimit)
				continue;
		}

		source.isInitialPosition = source.nodeHero == hlp->hero;
		source.updateInfo(hlp, gameInfo);

		//add accessible neighbouring nodes to the queue
		for(EPathfindingLayer layer = EPathfindingLayer::LAND; layer < EPathfindingLayer::NUM_LAYERS; layer.advance(1))
		{
			if(!hlp->isLayerAvailable(layer))
				continue;

			config->nodeStorage->calculateNeighbours(neighbourNodes, source, layer, config.get(), hlp);

			for(CGPathNode * neighbour : neighbourNodes)
			{
				if(neighbour->locked)
					continue;

				destination.setNode(gameInfo, neighbour);
				hlp = config->getOrCreatePathfinderHelper(destination, gameInfo);

				if(!hlp->isPatrolMovementAllowed(neighbour->coord))
					continue;

				/// Check transition without tile accessibility rules
				if(source.node->layer != neighbour->layer && !isLayerTransitionPossible())
					continue;

				destination.turn = turn;
				destination.movementLeft = movement;
				destination.cost = cost;
				destination.updateInfo(hlp, gameInfo);
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
		}

		//just add all passable teleport exits
		hlp = config->getOrCreatePathfinderHelper(source, gameInfo);

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

			destination.setNode(gameInfo, teleportNode);
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

TeleporterTilesVector CPathfinderHelper::getAllowedTeleportChannelExits(const TeleportChannelID & channelID) const
{
	TeleporterTilesVector allowedExits;

	for(const auto & objId : gameInfo.getTeleportChannelExits(channelID, hero->tempOwner))
	{
		const auto * obj = gameInfo.getObj(objId);
		if(dynamic_cast<const CGWhirlpool *>(obj))
		{
			auto pos = obj->getBlockedPos();
			for(const auto & p : pos)
			{
				ObjectInstanceID topObject = gameInfo.getTile(p)->topVisitableObj();
				if(topObject.hasValue() && gameInfo.getObj(topObject)->ID == obj->ID)
					allowedExits.push_back(p);
			}
		}
		else if(obj && CGTeleport::isExitPassable(gameInfo, hero, obj))
			allowedExits.push_back(obj->visitablePos());
	}

	return allowedExits;
}

TeleporterTilesVector CPathfinderHelper::getCastleGates(const PathNodeInfo & source) const
{
	TeleporterTilesVector allowedExits;

	for(const auto & town : gameInfo.getPlayerState(hero->tempOwner)->getTowns())
	{
		if(town->id != source.nodeObject->id && town->getVisitingHero() == nullptr
			&& town->hasBuilt(BuildingSubID::CASTLE_GATE))
		{
			allowedExits.push_back(town->visitablePos());
		}
	}

	return allowedExits;
}

TeleporterTilesVector CPathfinderHelper::getTeleportExits(const PathNodeInfo & source) const
{
	TeleporterTilesVector teleportationExits;

	const auto * objTeleport = dynamic_cast<const CGTeleport *>(source.nodeObject);
	if(isAllowedTeleportEntrance(objTeleport))
	{
		for(const auto & exit : getAllowedTeleportChannelExits(objTeleport->channel))
		{
			teleportationExits.push_back(exit);
		}
	}
	else if(options.useCastleGate && source.nodeObject->ID == Obj::TOWN && source.objectRelations != PlayerRelations::ENEMIES)
	{
		auto * town = dynamic_cast<const CGTownInstance *>(source.nodeObject);
		assert(town);
		if (town && town->getFactionID() == FactionID::INFERNO)
		{
			/// TODO: Find way to reuse CPlayerSpecificInfoCallback::getTownsInfo
			/// This may be handy if we allow to use teleportation to friendly towns
			for(const auto & exit : getCastleGates(source))
			{
				teleportationExits.push_back(exit);
			}
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
	if(!config->options.allowLayerTransitioningAfterBattle && source.node->action == EPathNodeAction::BATTLE)
		return false;

	switch(source.node->layer.toEnum())
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
	return gameInfo.guardingCreaturePosition(destination.node->coord) == destination.node->coord;
}

void CPathfinderHelper::initializePatrol()
{
	auto state = PATROL_NONE;

	if(hero->patrol.patrolling && !gameInfo.getPlayerState(hero->tempOwner)->human)
	{
		if(hero->patrol.patrolRadius)
		{
			state = PATROL_RADIUS;
			gameInfo.getTilesInRange(patrolTiles, hero->patrol.initialPos, hero->patrol.patrolRadius, ETileVisibility::REVEALED, std::optional<PlayerColor>(), int3::DIST_MANHATTAN);
		}
		else
			state = PATROL_LOCKED;
	}

	patrolState = state;
}

void CPathfinder::initializeGraph()
{
	INodeStorage * nodeStorage = config->nodeStorage.get();
	nodeStorage->initialize(config->options, gameInfo);
}

bool CPathfinderHelper::canMoveBetween(const int3 & a, const int3 & b) const
{
	return gameInfo.checkForVisitableDir(a, b);
}

bool CPathfinderHelper::isAllowedTeleportEntrance(const CGTeleport * obj) const
{
	if(!obj || !gameInfo.isTeleportEntrancePassable(obj, hero->tempOwner))
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
	return options.useTeleportTwoWay && gameInfo.isTeleportChannelBidirectional(obj->channel, hero->tempOwner);
}

bool CPathfinderHelper::addTeleportOneWay(const CGTeleport * obj) const
{
	if(options.useTeleportOneWay && gameInfo.isTeleportChannelUnidirectional(obj->channel, hero->tempOwner))
	{
		auto passableExits = CGTeleport::getPassableExits(gameInfo, hero, gameInfo.getTeleportChannelExits(obj->channel, hero->tempOwner));
		if(passableExits.size() == 1)
			return true;
	}
	return false;
}

bool CPathfinderHelper::addTeleportOneWayRandom(const CGTeleport * obj) const
{
	if(options.useTeleportOneWayRandom && gameInfo.isTeleportChannelUnidirectional(obj->channel, hero->tempOwner))
	{
		auto passableExits = CGTeleport::getPassableExits(gameInfo, hero, gameInfo.getTeleportChannelExits(obj->channel, hero->tempOwner));
		if(passableExits.size() > 1)
			return true;
	}
	return false;
}

bool CPathfinderHelper::addTeleportWhirlpool(const CGWhirlpool * obj) const
{
	return options.useTeleportWhirlpool && (whirlpoolProtection || options.forceUseTeleportWhirlpool) && obj;
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
		return options.originalFlyRules && source.node->accessible == EPathAccessibility::ACCESSIBLE;
	}

	return true;
}

int CPathfinderHelper::getGuardiansCount(int3 tile) const
{
	return gameInfo.getGuardingCreatures(tile).size();
}

CPathfinderHelper::CPathfinderHelper(const IGameInfoCallback & gameInfo, const CGHeroInstance * Hero, const PathfinderOptions & Options):
	gameInfo(gameInfo),
	turn(-1),
	owner(Hero->tempOwner),
	hero(Hero),
	options(Options),
	canCastFly(false),
	canCastWaterWalk(false)
{
	turnsInfo.reserve(16);
	updateTurnInfo();
	initializePatrol();

	whirlpoolProtection = Hero->hasBonusOfType(BonusType::WHIRLPOOL_PROTECTION);

	if (options.canUseCast)
	{
		for (const auto & spell : LIBRARY->spellh->objects)
		{
			if (!spell || !spell->isAdventure())
				continue;

			if(spell->getAdventureMechanics().givesBonus(hero, BonusType::WATER_WALKING) && hero->canCastThisSpell(spell.get()) && hero->mana >= hero->getSpellCost(spell.get()))
				canCastWaterWalk = true;

			if(spell->getAdventureMechanics().givesBonus(hero, BonusType::FLYING_MOVEMENT) && hero->canCastThisSpell(spell.get()) && hero->mana >= hero->getSpellCost(spell.get()))
				canCastFly = true;
		}
	}
}

CPathfinderHelper::~CPathfinderHelper() = default;

void CPathfinderHelper::updateTurnInfo(const int Turn)
{
	if(turn != Turn)
	{
		turn = Turn;
		if(turn >= turnsInfo.size())
			turnsInfo.push_back(hero->getTurnInfo(turn));
	}
}

bool CPathfinderHelper::isLayerAvailable(const EPathfindingLayer & layer) const
{
	switch(layer.toEnum())
	{
	case EPathfindingLayer::AIR:
		if(!options.useFlying)
			return false;

		if(canCastFly && options.canUseCast)
			return true;

		break;

	case EPathfindingLayer::WATER:
		if(!options.useWaterWalking)
			return false;

		if(canCastWaterWalk && options.canUseCast)
			return true;

		break;
	}

	return turnsInfo[turn]->isLayerAvailable(layer);
}

const TurnInfo * CPathfinderHelper::getTurnInfo() const
{
	return turnsInfo[turn].get();
}

int CPathfinderHelper::getMaxMovePoints(const EPathfindingLayer & layer) const
{
	return turnsInfo[turn]->getMaxMovePoints(layer);
}

void CPathfinderHelper::getNeighbours(
	const TerrainTile & sourceTile,
	const int3 & srcCoord,
	NeighbourTilesVector & vec,
	const boost::logic::tribool & onLand,
	const bool limitCoastSailing) const
{
	const TerrainType * sourceTerrain = sourceTile.getTerrain();

	static constexpr std::array dirs = {
		int3(-1, +1, +0),	int3(0, +1, +0),	int3(+1, +1, +0),
		int3(-1, +0, +0),	/* source pos */	int3(+1, +0, +0),
		int3(-1, -1, +0),	int3(0, -1, +0),	int3(+1, -1, +0)
	};

	for(const auto & dir : dirs)
	{
		const int3 destCoord = srcCoord + dir;
		if(!gameInfo.isInTheMap(destCoord))
			continue;

		const TerrainTile * destTile = gameInfo.getTile(destCoord);
		const TerrainType * destTerrain = destTile->getTerrain();
		if(!destTerrain->isPassable())
			continue;

		/// Following condition let us avoid diagonal movement over coast when sailing
		if(sourceTerrain->isWater() && limitCoastSailing && destTerrain->isWater() && dir.x && dir.y) //diagonal move through water
		{
			const int3 horizontalNeighbour = srcCoord + int3{dir.x, 0, 0};
			const int3 verticalNeighbour = srcCoord + int3{0, dir.y, 0};
			if(gameInfo.getTile(horizontalNeighbour)->isLand() || gameInfo.getTile(verticalNeighbour)->isLand())
				continue;
		}

		if(indeterminate(onLand) || onLand == destTerrain->isLand())
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
		isSailLayer = hero->inBoat() && hero->getBoat()->layer == EPathfindingLayer::SAIL && dt->isWater();
	else
		isSailLayer = static_cast<bool>(isDstSailLayer);

	bool isWaterLayer;
	if(indeterminate(isDstWaterLayer))
		isWaterLayer = ((hero->inBoat() && hero->getBoat()->layer == EPathfindingLayer::WATER) || ti->hasWaterWalking()) && dt->isWater();
	else
		isWaterLayer = static_cast<bool>(isDstWaterLayer);
	
	bool isAirLayer = (hero->inBoat() && hero->getBoat()->layer == EPathfindingLayer::AIR) || ti->hasFlyingMovement();

	int movementCost = getTileMovementCost(*dt, *ct, ti);
	if(isSailLayer)
	{
		if(ct->hasFavorableWinds())
			movementCost = static_cast<int>(movementCost * 2.0 / 3);
	}
	else if(isAirLayer)
	{
		int baseCost = gameInfo.getSettings().getInteger(EGameSettings::HEROES_MOVEMENT_COST_BASE);
		vstd::amin(movementCost, baseCost + ti->getFlyingMovementValue());
	}
	else if(isWaterLayer && ti->hasWaterWalking())
		movementCost = static_cast<int>(movementCost * (100.0 + ti->getWaterWalkingValue()) / 100.0);

	if(src.x != dst.x && src.y != dst.y) //it's diagonal move
	{
		int old = movementCost;
		movementCost = static_cast<int>(movementCost * M_SQRT2);
		//diagonal move costs too much but normal move is possible - allow diagonal move for remaining move points
		// https://heroes.thelazy.net/index.php/Movement#Diagonal_move_exception
		if(movementCost > remainingMovePoints && remainingMovePoints >= old)
		{
			return remainingMovePoints;
		}
	}

	//it might be the last tile - if no further move possible we take all move points
	const int pointsLeft = remainingMovePoints - movementCost;
	if(checkLast && pointsLeft > 0)
	{
		int minimalNextMoveCost = getTileMovementCost(*dt, *ct, ti);

		if (pointsLeft < minimalNextMoveCost)
			return remainingMovePoints;
	}

	return movementCost;
}

ui32 CPathfinderHelper::getTileMovementCost(const TerrainTile & dest, const TerrainTile & from, const TurnInfo * ti) const
{
	//if there is road both on dest and src tiles - use src road movement cost
	if(dest.hasRoad() && from.hasRoad())
		return from.getRoad()->movementCost;

	int baseMovementCost = ti->getMovementCostBase();
	int terrainMoveCost = from.getTerrain()->moveCost;
	int terrainDiscout = ti->getRoughTerrainDiscountValue();

	int costWithPathfinding = std::max(baseMovementCost, terrainMoveCost - terrainDiscout);

	//if hero can move without penalty - either all-native army, or creatures like Nomads in army
	if(ti->hasNoTerrainPenalty(from.getTerrainID()))
	{
		int baseCost = gameInfo.getSettings().getInteger(EGameSettings::HEROES_MOVEMENT_COST_BASE);
		return std::min(baseCost, costWithPathfinding);
	}

	return costWithPathfinding;
}

VCMI_LIB_NAMESPACE_END
