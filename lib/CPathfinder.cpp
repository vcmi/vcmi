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

#include "CHeroHandler.h"
#include "mapping/CMap.h"
#include "CGameState.h"
#include "mapObjects/CGHeroInstance.h"
#include "GameConstants.h"
#include "CStopWatch.h"
#include "CConfigHandler.h"
#include "CPlayerState.h"
#include "PathfinderUtil.h"

VCMI_LIB_NAMESPACE_BEGIN

bool canSeeObj(const CGObjectInstance * obj)
{
	/// Pathfinder should ignore placed events
	return obj != nullptr && obj->ID != Obj::EVENT;
}

void NodeStorage::initialize(const PathfinderOptions & options, const CGameState * gs)
{
	//TODO: fix this code duplication with AINodeStorage::initialize, problem is to keep `resetTile` inline

	int3 pos;
	const PlayerColor player = out.hero->tempOwner;
	const int3 sizes = gs->getMapSize();
	const auto fow = static_cast<const CGameInfoCallback *>(gs)->getPlayerTeam(player)->fogOfWarMap;

	//make 200% sure that these are loop invariants (also a bit shorter code), let compiler do the rest(loop unswitching)
	const bool useFlying = options.useFlying;
	const bool useWaterWalking = options.useWaterWalking;

	for(pos.z=0; pos.z < sizes.z; ++pos.z)
	{
		for(pos.x=0; pos.x < sizes.x; ++pos.x)
		{
			for(pos.y=0; pos.y < sizes.y; ++pos.y)
			{
				const TerrainTile tile = gs->map->getTile(pos);
				if(tile.terType->isWater())
				{
					resetTile(pos, ELayer::SAIL, PathfinderUtil::evaluateAccessibility<ELayer::SAIL>(pos, tile, fow, player, gs));
					if(useFlying)
						resetTile(pos, ELayer::AIR, PathfinderUtil::evaluateAccessibility<ELayer::AIR>(pos, tile, fow, player, gs));
					if(useWaterWalking)
						resetTile(pos, ELayer::WATER, PathfinderUtil::evaluateAccessibility<ELayer::WATER>(pos, tile, fow, player, gs));
				}
				if(tile.terType->isLand())
				{
					resetTile(pos, ELayer::LAND, PathfinderUtil::evaluateAccessibility<ELayer::LAND>(pos, tile, fow, player, gs));
					if(useFlying)
						resetTile(pos, ELayer::AIR, PathfinderUtil::evaluateAccessibility<ELayer::AIR>(pos, tile, fow, player, gs));
				}
			}
		}
	}
}

std::vector<CGPathNode *> NodeStorage::calculateNeighbours(
	const PathNodeInfo & source,
	const PathfinderConfig * pathfinderConfig,
	const CPathfinderHelper * pathfinderHelper)
{
	std::vector<CGPathNode *> neighbours;
	neighbours.reserve(16);
	auto accessibleNeighbourTiles = pathfinderHelper->getNeighbourTiles(source);

	for(auto & neighbour : accessibleNeighbourTiles)
	{
		for(EPathfindingLayer i = EPathfindingLayer::LAND; i <= EPathfindingLayer::AIR; i.advance(1))
		{
			auto * node = getNode(neighbour, i);

			if(node->accessible == CGPathNode::NOT_SET)
				continue;

			neighbours.push_back(node);
		}
	}

	return neighbours;
}

std::vector<CGPathNode *> NodeStorage::calculateTeleportations(
	const PathNodeInfo & source,
	const PathfinderConfig * pathfinderConfig,
	const CPathfinderHelper * pathfinderHelper)
{
	std::vector<CGPathNode *> neighbours;

	if(!source.isNodeObjectVisitable())
		return neighbours;

	auto accessibleExits = pathfinderHelper->getTeleportExits(source);

	for(auto & neighbour : accessibleExits)
	{
		auto * node = getNode(neighbour, source.node->layer);

		neighbours.push_back(node);
	}

	return neighbours;
}

std::vector<int3> CPathfinderHelper::getNeighbourTiles(const PathNodeInfo & source) const
{
	std::vector<int3> neighbourTiles;
	neighbourTiles.reserve(16);

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

NodeStorage::NodeStorage(CPathsInfo & pathsInfo, const CGHeroInstance * hero)
	:out(pathsInfo)
{
	out.hero = hero;
	out.hpos = hero->visitablePos();
}

void NodeStorage::resetTile(const int3 & tile, const EPathfindingLayer & layer, CGPathNode::EAccessibility accessibility)
{
	getNode(tile, layer)->update(tile, layer, accessibility);
}

std::vector<CGPathNode *> NodeStorage::getInitialNodes()
{
	auto * initialNode = getNode(out.hpos, out.hero->boat ? EPathfindingLayer::SAIL : EPathfindingLayer::LAND);

	initialNode->turns = 0;
	initialNode->moveRemains = out.hero->movement;
	initialNode->setCost(0.0);

	if(!initialNode->coord.valid())
	{
		initialNode->coord = out.hpos;
	}

	return std::vector<CGPathNode *> { initialNode };
}

void NodeStorage::commit(CDestinationNodeInfo & destination, const PathNodeInfo & source)
{
	assert(destination.node != source.node->theNodeBefore); //two tiles can't point to each other
	destination.node->setCost(destination.cost);
	destination.node->moveRemains = destination.movementLeft;
	destination.node->turns = destination.turn;
	destination.node->theNodeBefore = source.node;
	destination.node->action = destination.action;
}

PathfinderOptions::PathfinderOptions()
{
	useFlying = settings["pathfinder"]["layers"]["flying"].Bool();
	useWaterWalking = settings["pathfinder"]["layers"]["waterWalking"].Bool();
	useEmbarkAndDisembark = settings["pathfinder"]["layers"]["sailing"].Bool();
	useTeleportTwoWay = settings["pathfinder"]["teleports"]["twoWay"].Bool();
	useTeleportOneWay = settings["pathfinder"]["teleports"]["oneWay"].Bool();
	useTeleportOneWayRandom = settings["pathfinder"]["teleports"]["oneWayRandom"].Bool();
	useTeleportWhirlpool = settings["pathfinder"]["teleports"]["whirlpool"].Bool();

	useCastleGate = settings["pathfinder"]["teleports"]["castleGate"].Bool();

	lightweightFlyingMode = settings["pathfinder"]["lightweightFlyingMode"].Bool();
	oneTurnSpecialLayersLimit = settings["pathfinder"]["oneTurnSpecialLayersLimit"].Bool();
	originalMovementRules = settings["pathfinder"]["originalMovementRules"].Bool();
}

void MovementCostRule::process(
	const PathNodeInfo & source,
	CDestinationNodeInfo & destination,
	const PathfinderConfig * pathfinderConfig,
	CPathfinderHelper * pathfinderHelper) const
{
	float costAtNextTile = destination.cost;
	int turnAtNextTile = destination.turn;
	int moveAtNextTile = destination.movementLeft;
	int cost = pathfinderHelper->getMovementCost(source, destination, moveAtNextTile);
	int remains = moveAtNextTile - cost;
	int sourceLayerMaxMovePoints = pathfinderHelper->getMaxMovePoints(source.node->layer);

	if(remains < 0)
	{
		//occurs rarely, when hero with low movepoints tries to leave the road
		costAtNextTile += static_cast<float>(moveAtNextTile) / sourceLayerMaxMovePoints;//we spent all points of current turn
		pathfinderHelper->updateTurnInfo(++turnAtNextTile);

		int destinationLayerMaxMovePoints = pathfinderHelper->getMaxMovePoints(destination.node->layer);

		moveAtNextTile = destinationLayerMaxMovePoints;

		cost = pathfinderHelper->getMovementCost(source, destination, moveAtNextTile); //cost must be updated, movement points changed :(
		remains = moveAtNextTile - cost;
	}

	if(destination.action == CGPathNode::EMBARK || destination.action == CGPathNode::DISEMBARK)
	{
		/// FREE_SHIP_BOARDING bonus only remove additional penalty
		/// land <-> sail transition still cost movement points as normal movement
		remains = pathfinderHelper->movementPointsAfterEmbark(moveAtNextTile, cost, (destination.action == CGPathNode::DISEMBARK));
		cost = moveAtNextTile - remains;
	}

	costAtNextTile += static_cast<float>(cost) / sourceLayerMaxMovePoints;

	destination.cost = costAtNextTile;
	destination.turn = turnAtNextTile;
	destination.movementLeft = remains;

	if(destination.isBetterWay() &&
		((source.node->turns == turnAtNextTile && remains) || pathfinderHelper->passOneTurnLimitCheck(source)))
	{
		pathfinderConfig->nodeStorage->commit(destination, source);

		return;
	}

	destination.blocked = true;
}

PathfinderConfig::PathfinderConfig(std::shared_ptr<INodeStorage> nodeStorage, std::vector<std::shared_ptr<IPathfindingRule>> rules):
	nodeStorage(std::move(nodeStorage)),
	rules(std::move(rules))
{
}

std::vector<std::shared_ptr<IPathfindingRule>> SingleHeroPathfinderConfig::buildRuleSet()
{
	return std::vector<std::shared_ptr<IPathfindingRule>>{
		std::make_shared<LayerTransitionRule>(),
			std::make_shared<DestinationActionRule>(),
			std::make_shared<MovementToDestinationRule>(),
			std::make_shared<MovementCostRule>(),
			std::make_shared<MovementAfterDestinationRule>()
	};
}

SingleHeroPathfinderConfig::SingleHeroPathfinderConfig(CPathsInfo & out, CGameState * gs, const CGHeroInstance * hero)
	: PathfinderConfig(std::make_shared<NodeStorage>(out, hero), buildRuleSet())
{
	pathfinderHelper = std::make_unique<CPathfinderHelper>(gs, hero, options);
}

CPathfinderHelper * SingleHeroPathfinderConfig::getOrCreatePathfinderHelper(const PathNodeInfo & source, CGameState * gs)
{
	return pathfinderHelper.get();
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
			if(teleportNode->accessible == CGPathNode::BLOCKED)
				continue;

			destination.setNode(gamestate, teleportNode);
			destination.turn = turn;
			destination.movementLeft = movement;
			destination.cost = cost;

			if(destination.isBetterWay())
			{
				destination.action = getTeleportDestAction();
				config->nodeStorage->commit(destination, source);

				if(destination.node->action == CGPathNode::TELEPORT_NORMAL)
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
	if(source.node->action == CGPathNode::BATTLE)
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

void LayerTransitionRule::process(
	const PathNodeInfo & source,
	CDestinationNodeInfo & destination,
	const PathfinderConfig * pathfinderConfig,
	CPathfinderHelper * pathfinderHelper) const
{
	if(source.node->layer == destination.node->layer)
		return;

	switch(source.node->layer)
	{
	case EPathfindingLayer::LAND:
		if(destination.node->layer == EPathfindingLayer::SAIL)
		{
			/// Cannot enter empty water tile from land -> it has to be visitable
			if(destination.node->accessible == CGPathNode::ACCESSIBLE)
				destination.blocked = true;
		}

		break;

	case EPathfindingLayer::SAIL:
		//tile must be accessible -> exception: unblocked blockvis tiles -> clear but guarded by nearby monster coast
		if((destination.node->accessible != CGPathNode::ACCESSIBLE && (destination.node->accessible != CGPathNode::BLOCKVIS || destination.tile->blocked))
			|| destination.tile->visitable)  //TODO: passableness problem -> town says it's passable (thus accessible) but we obviously can't disembark onto town gate
		{
			destination.blocked = true;
		}

		break;

	case EPathfindingLayer::AIR:
		if(pathfinderConfig->options.originalMovementRules)
		{
			if((source.node->accessible != CGPathNode::ACCESSIBLE &&
				source.node->accessible != CGPathNode::VISITABLE) &&
				(destination.node->accessible != CGPathNode::VISITABLE &&
				 destination.node->accessible != CGPathNode::ACCESSIBLE))
			{
				destination.blocked = true;
			}
		}
		else if(destination.node->accessible != CGPathNode::ACCESSIBLE)
		{
			/// Hero that fly can only land on accessible tiles
			if(destination.nodeObject)
				destination.blocked = true;
		}

		break;

	case EPathfindingLayer::WATER:
		if(destination.node->accessible != CGPathNode::ACCESSIBLE && destination.node->accessible != CGPathNode::VISITABLE)
		{
			/// Hero that walking on water can transit to accessible and visitable tiles
			/// Though hero can't interact with blocking visit objects while standing on water
			destination.blocked = true;
		}

		break;
	}
}

PathfinderBlockingRule::BlockingReason MovementToDestinationRule::getBlockingReason(
	const PathNodeInfo & source,
	const CDestinationNodeInfo & destination,
	const PathfinderConfig * pathfinderConfig,
	const CPathfinderHelper * pathfinderHelper) const
{

	if(destination.node->accessible == CGPathNode::BLOCKED)
		return BlockingReason::DESTINATION_BLOCKED;

	switch(destination.node->layer)
	{
	case EPathfindingLayer::LAND:
		if(!pathfinderHelper->canMoveBetween(source.coord, destination.coord))
			return BlockingReason::DESTINATION_BLOCKED;

		if(source.guarded)
		{
			if(!(pathfinderConfig->options.originalMovementRules && source.node->layer == EPathfindingLayer::AIR) &&
				!destination.isGuardianTile) // Can step into tile of guard
			{
				return BlockingReason::SOURCE_GUARDED;
			}
		}

		break;

	case EPathfindingLayer::SAIL:
		if(!pathfinderHelper->canMoveBetween(source.coord, destination.coord))
			return BlockingReason::DESTINATION_BLOCKED;

		if(source.guarded)
		{
			// Hero embarked a boat standing on a guarded tile -> we must allow to move away from that tile
			if(source.node->action != CGPathNode::EMBARK && !destination.isGuardianTile)
				return BlockingReason::SOURCE_GUARDED;
		}

		if(source.node->layer == EPathfindingLayer::LAND)
		{
			if(!destination.isNodeObjectVisitable())
				return BlockingReason::DESTINATION_BLOCKED;

			if(destination.nodeObject->ID != Obj::BOAT && !destination.nodeHero)
				return BlockingReason::DESTINATION_BLOCKED;
		}
		else if(destination.isNodeObjectVisitable() && destination.nodeObject->ID == Obj::BOAT)
		{
			/// Hero in boat can't visit empty boats
			return BlockingReason::DESTINATION_BLOCKED;
		}

		break;

	case EPathfindingLayer::WATER:
		if(!pathfinderHelper->canMoveBetween(source.coord, destination.coord)
			|| destination.node->accessible != CGPathNode::ACCESSIBLE)
		{
			return BlockingReason::DESTINATION_BLOCKED;
		}

		if(destination.guarded)
			return BlockingReason::DESTINATION_BLOCKED;

		break;
	}

	return BlockingReason::NONE;
}


void MovementAfterDestinationRule::process(
	const PathNodeInfo & source,
	CDestinationNodeInfo & destination,
	const PathfinderConfig * config,
	CPathfinderHelper * pathfinderHelper) const
{
	auto blocker = getBlockingReason(source, destination, config, pathfinderHelper);

	if(blocker == BlockingReason::DESTINATION_GUARDED && destination.action == CGPathNode::ENodeAction::BATTLE)
	{
		return; // allow bypass guarded tile but only in direction of guard, a bit UI related thing
	}

	destination.blocked = blocker != BlockingReason::NONE;
}

PathfinderBlockingRule::BlockingReason MovementAfterDestinationRule::getBlockingReason(
	const PathNodeInfo & source,
	const CDestinationNodeInfo & destination,
	const PathfinderConfig * config,
	const CPathfinderHelper * pathfinderHelper) const
{
	switch(destination.action)
	{
	/// TODO: Investigate what kind of limitation is possible to apply on movement from visitable tiles
	/// Likely in many cases we don't need to add visitable tile to queue when hero don't fly
	case CGPathNode::VISIT:
	{
		/// For now we only add visitable tile into queue when it's teleporter that allow transit
		/// Movement from visitable tile when hero is standing on it is possible into any layer
		const auto * objTeleport = dynamic_cast<const CGTeleport *>(destination.nodeObject);
		if(pathfinderHelper->isAllowedTeleportEntrance(objTeleport))
		{
			/// For now we'll always allow transit over teleporters
			/// Transit over whirlpools only allowed when hero protected
			return BlockingReason::NONE;
		}
		else if(destination.nodeObject->ID == Obj::GARRISON
			|| destination.nodeObject->ID == Obj::GARRISON2
			|| destination.nodeObject->ID == Obj::BORDER_GATE)
		{
			/// Transit via unguarded garrisons is always possible
			return BlockingReason::NONE;
		}

		return BlockingReason::DESTINATION_VISIT;
	}

	case CGPathNode::BLOCKING_VISIT:
		return destination.guarded
			? BlockingReason::DESTINATION_GUARDED
			: BlockingReason::DESTINATION_BLOCKVIS;

	case CGPathNode::NORMAL:
		return BlockingReason::NONE;

	case CGPathNode::EMBARK:
		if(pathfinderHelper->options.useEmbarkAndDisembark)
			return BlockingReason::NONE;

		return BlockingReason::DESTINATION_BLOCKED;

	case CGPathNode::DISEMBARK:
		if(pathfinderHelper->options.useEmbarkAndDisembark)
			return destination.guarded ? BlockingReason::DESTINATION_GUARDED : BlockingReason::NONE;

		return BlockingReason::DESTINATION_BLOCKED;

	case CGPathNode::BATTLE:
		/// Movement after BATTLE action only possible from guarded tile to guardian tile
		if(destination.guarded)
			return BlockingReason::DESTINATION_GUARDED;

		break;
	}

	return BlockingReason::DESTINATION_BLOCKED;
}

void DestinationActionRule::process(
	const PathNodeInfo & source,
	CDestinationNodeInfo & destination,
	const PathfinderConfig * pathfinderConfig,
	CPathfinderHelper * pathfinderHelper) const
{
	if(destination.action != CGPathNode::ENodeAction::UNKNOWN)
	{
#ifdef VCMI_TRACE_PATHFINDER
		logAi->trace("Accepted precalculated action at %s", destination.coord.toString());
#endif
		return;
	}

	CGPathNode::ENodeAction action = CGPathNode::NORMAL;
	const auto * hero = pathfinderHelper->hero;

	switch(destination.node->layer)
	{
	case EPathfindingLayer::LAND:
		if(source.node->layer == EPathfindingLayer::SAIL)
		{
			// TODO: Handle dismebark into guarded areaa
			action = CGPathNode::DISEMBARK;
			break;
		}

		/// don't break - next case shared for both land and sail layers
		FALLTHROUGH

	case EPathfindingLayer::SAIL:
		if(destination.isNodeObjectVisitable())
		{
			auto objRel = destination.objectRelations;

			if(destination.nodeObject->ID == Obj::BOAT)
				action = CGPathNode::EMBARK;
			else if(destination.nodeHero)
			{
				if(destination.heroRelations == PlayerRelations::ENEMIES)
					action = CGPathNode::BATTLE;
				else
					action = CGPathNode::BLOCKING_VISIT;
			}
			else if(destination.nodeObject->ID == Obj::TOWN)
			{
				if(destination.nodeObject->passableFor(hero->tempOwner))
					action = CGPathNode::VISIT;
				else if(objRel == PlayerRelations::ENEMIES)
					action = CGPathNode::BATTLE;
			}
			else if(destination.nodeObject->ID == Obj::GARRISON || destination.nodeObject->ID == Obj::GARRISON2)
			{
				if(destination.nodeObject->passableFor(hero->tempOwner))
				{
					if(destination.guarded)
						action = CGPathNode::BATTLE;
				}
				else if(objRel == PlayerRelations::ENEMIES)
					action = CGPathNode::BATTLE;
			}
			else if(destination.nodeObject->ID == Obj::BORDER_GATE)
			{
				if(destination.nodeObject->passableFor(hero->tempOwner))
				{
					if(destination.guarded)
						action = CGPathNode::BATTLE;
				}
				else
					action = CGPathNode::BLOCKING_VISIT;
			}
			else if(destination.isGuardianTile)
				action = CGPathNode::BATTLE;
			else if(destination.nodeObject->blockVisit && !(pathfinderConfig->options.useCastleGate && destination.nodeObject->ID == Obj::TOWN))
				action = CGPathNode::BLOCKING_VISIT;

			if(action == CGPathNode::NORMAL)
			{
				if(destination.guarded)
					action = CGPathNode::BATTLE;
				else
					action = CGPathNode::VISIT;
			}
		}
		else if(destination.guarded)
			action = CGPathNode::BATTLE;

		break;
	}

	destination.action = action;
}

CGPathNode::ENodeAction CPathfinder::getTeleportDestAction() const
{
	CGPathNode::ENodeAction action = CGPathNode::TELEPORT_NORMAL;

	if(destination.isNodeObjectVisitable() && destination.nodeHero)
	{
		if(destination.heroRelations == PlayerRelations::ENEMIES)
			action = CGPathNode::TELEPORT_BATTLE;
		else
			action = CGPathNode::TELEPORT_BLOCKING_VISIT;
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
			gs->getTilesInRange(patrolTiles, hero->patrol.initialPos, hero->patrol.patrolRadius, boost::optional<PlayerColor>(), 0, int3::DIST_MANHATTAN);
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
	return options.useTeleportWhirlpool && hasBonusOfType(Bonus::WHIRLPOOL_PROTECTION) && obj;
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
		return options.originalMovementRules && source.node->accessible == CGPathNode::ACCESSIBLE;
	}

	return true;
}

TurnInfo::BonusCache::BonusCache(const TConstBonusListPtr & bl)
{
	for(const auto & terrain : VLC->terrainTypeHandler->objects)
	{
		noTerrainPenalty.push_back(static_cast<bool>(
				bl->getFirst(Selector::type()(Bonus::NO_TERRAIN_PENALTY).And(Selector::subtype()(terrain->getIndex())))));
	}

	freeShipBoarding = static_cast<bool>(bl->getFirst(Selector::type()(Bonus::FREE_SHIP_BOARDING)));
	flyingMovement = static_cast<bool>(bl->getFirst(Selector::type()(Bonus::FLYING_MOVEMENT)));
	flyingMovementVal = bl->valOfBonuses(Selector::type()(Bonus::FLYING_MOVEMENT));
	waterWalking = static_cast<bool>(bl->getFirst(Selector::type()(Bonus::WATER_WALKING)));
	waterWalkingVal = bl->valOfBonuses(Selector::type()(Bonus::WATER_WALKING));
	pathfindingVal = bl->valOfBonuses(Selector::type()(Bonus::ROUGH_TERRAIN_DISCOUNT));
}

TurnInfo::TurnInfo(const CGHeroInstance * Hero, const int turn):
	hero(Hero),
	maxMovePointsLand(-1),
	maxMovePointsWater(-1),
	turn(turn)
{
	bonuses = hero->getAllBonuses(Selector::days(turn), Selector::all, nullptr, "");
	bonusCache = std::make_unique<BonusCache>(bonuses);
	nativeTerrain = hero->getNativeTerrain();
}

bool TurnInfo::isLayerAvailable(const EPathfindingLayer & layer) const
{
	switch(layer)
	{
	case EPathfindingLayer::AIR:
		if(!hasBonusOfType(Bonus::FLYING_MOVEMENT))
			return false;

		break;

	case EPathfindingLayer::WATER:
		if(!hasBonusOfType(Bonus::WATER_WALKING))
			return false;

		break;
	}

	return true;
}

bool TurnInfo::hasBonusOfType(Bonus::BonusType type, int subtype) const
{
	switch(type)
	{
	case Bonus::FREE_SHIP_BOARDING:
		return bonusCache->freeShipBoarding;
	case Bonus::FLYING_MOVEMENT:
		return bonusCache->flyingMovement;
	case Bonus::WATER_WALKING:
		return bonusCache->waterWalking;
	case Bonus::NO_TERRAIN_PENALTY:
		return bonusCache->noTerrainPenalty[subtype];
	}

	return static_cast<bool>(
			bonuses->getFirst(Selector::type()(type).And(Selector::subtype()(subtype))));
}

int TurnInfo::valOfBonuses(Bonus::BonusType type, int subtype) const
{
	switch(type)
	{
	case Bonus::FLYING_MOVEMENT:
		return bonusCache->flyingMovementVal;
	case Bonus::WATER_WALKING:
		return bonusCache->waterWalkingVal;
	case Bonus::ROUGH_TERRAIN_DISCOUNT:
		return bonusCache->pathfindingVal;
	}

	return bonuses->valOfBonuses(Selector::type()(type).And(Selector::subtype()(subtype)));
}

int TurnInfo::getMaxMovePoints(const EPathfindingLayer & layer) const
{
	if(maxMovePointsLand == -1)
		maxMovePointsLand = hero->maxMovePointsCached(true, this);
	if(maxMovePointsWater == -1)
		maxMovePointsWater = hero->maxMovePointsCached(false, this);

	return layer == EPathfindingLayer::SAIL ? maxMovePointsWater : maxMovePointsLand;
}

void TurnInfo::updateHeroBonuses(Bonus::BonusType type, const CSelector& sel) const
{
	switch(type)
	{
	case Bonus::FREE_SHIP_BOARDING:
		bonusCache->freeShipBoarding = static_cast<bool>(bonuses->getFirst(Selector::type()(Bonus::FREE_SHIP_BOARDING)));
		break;
	case Bonus::FLYING_MOVEMENT:
		bonusCache->flyingMovement = static_cast<bool>(bonuses->getFirst(Selector::type()(Bonus::FLYING_MOVEMENT)));
		bonusCache->flyingMovementVal = bonuses->valOfBonuses(Selector::type()(Bonus::FLYING_MOVEMENT));
		break;
	case Bonus::WATER_WALKING:
		bonusCache->waterWalking = static_cast<bool>(bonuses->getFirst(Selector::type()(Bonus::WATER_WALKING)));
		bonusCache->waterWalkingVal = bonuses->valOfBonuses(Selector::type()(Bonus::WATER_WALKING));
		break;
	case Bonus::ROUGH_TERRAIN_DISCOUNT:
		bonusCache->pathfindingVal = bonuses->valOfBonuses(Selector::type()(Bonus::ROUGH_TERRAIN_DISCOUNT));
		break;
	default:
		bonuses = hero->getAllBonuses(Selector::days(turn), Selector::all, nullptr, "");
	}
}

CPathfinderHelper::CPathfinderHelper(CGameState * gs, const CGHeroInstance * Hero, const PathfinderOptions & Options)
	: CGameInfoCallback(gs, boost::optional<PlayerColor>()), turn(-1), hero(Hero), options(Options), owner(Hero->tempOwner)
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

bool CPathfinderHelper::hasBonusOfType(const Bonus::BonusType type, const int subtype) const
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
	const int3 & src,
	const int3 & dst,
	const TerrainTile * ct,
	const TerrainTile * dt,
	const int remainingMovePoints,
	const bool checkLast) const
{
	if(src == dst) //same tile
		return 0;

	const auto * ti = getTurnInfo();

	if(ct == nullptr || dt == nullptr)
	{
		ct = hero->cb->getTile(src);
		dt = hero->cb->getTile(dst);
	}

	int ret = hero->getTileCost(*dt, *ct, ti);
	if(hero->boat != nullptr && dt->terType->isWater())
	{
		if(ct->hasFavorableWinds())
			ret = static_cast<int>(ret * 2.0 / 3);
	}
	else if(ti->hasBonusOfType(Bonus::FLYING_MOVEMENT))
		vstd::amin(ret, GameConstants::BASE_MOVEMENT_COST + ti->valOfBonuses(Bonus::FLYING_MOVEMENT));
	else if(hero->boat == nullptr && dt->terType->isWater() && ti->hasBonusOfType(Bonus::WATER_WALKING))
		ret = static_cast<int>(ret * (100.0 + ti->valOfBonuses(Bonus::WATER_WALKING)) / 100.0);

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
		for(auto & elem : vec)
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

int3 CGPath::startPos() const
{
	return nodes[nodes.size()-1].coord;
}

int3 CGPath::endPos() const
{
	return nodes[0].coord;
}

CPathsInfo::CPathsInfo(const int3 & Sizes, const CGHeroInstance * hero_)
	: sizes(Sizes), hero(hero_)
{
	nodes.resize(boost::extents[ELayer::NUM_LAYERS][sizes.z][sizes.x][sizes.y]);
}

CPathsInfo::~CPathsInfo() = default;

const CGPathNode * CPathsInfo::getPathInfo(const int3 & tile) const
{
	assert(vstd::iswithin(tile.x, 0, sizes.x));
	assert(vstd::iswithin(tile.y, 0, sizes.y));
	assert(vstd::iswithin(tile.z, 0, sizes.z));

	return getNode(tile);
}

bool CPathsInfo::getPath(CGPath & out, const int3 & dst) const
{
	out.nodes.clear();
	const CGPathNode * curnode = getNode(dst);
	if(!curnode->theNodeBefore)
		return false;

	while(curnode)
	{
		const CGPathNode cpn = * curnode;
		curnode = curnode->theNodeBefore;
		out.nodes.push_back(cpn);
	}
	return true;
}

const CGPathNode * CPathsInfo::getNode(const int3 & coord) const
{
	const auto * landNode = &nodes[ELayer::LAND][coord.z][coord.x][coord.y];
	if(landNode->reachable())
		return landNode;
	else
		return &nodes[ELayer::SAIL][coord.z][coord.x][coord.y];
}

PathNodeInfo::PathNodeInfo()
	: node(nullptr), nodeObject(nullptr), tile(nullptr), coord(-1, -1, -1), guarded(false),	isInitialPosition(false)
{
}

void PathNodeInfo::setNode(CGameState * gs, CGPathNode * n)
{
	node = n;

	if(coord != node->coord)
	{
		assert(node->coord.valid());

		coord = node->coord;
		tile = gs->getTile(coord);
		nodeObject = tile->topVisitableObj();
		
		if(nodeObject && nodeObject->ID == Obj::HERO)
		{
			nodeHero = dynamic_cast<const CGHeroInstance *>(nodeObject);
			nodeObject = tile->topVisitableObj(true);

			if(!nodeObject)
				nodeObject = nodeHero;
		}
		else
		{
			nodeHero = nullptr;
		}
	}

	guarded = false;
}

void PathNodeInfo::updateInfo(CPathfinderHelper * hlp, CGameState * gs)
{
	if(gs->guardingCreaturePosition(node->coord).valid() && !isInitialPosition)
	{
		guarded = true;
	}

	if(nodeObject)
	{
		objectRelations = gs->getPlayerRelations(hlp->owner, nodeObject->tempOwner);
	}

	if(nodeHero)
	{
		heroRelations = gs->getPlayerRelations(hlp->owner, nodeHero->tempOwner);
	}
}

CDestinationNodeInfo::CDestinationNodeInfo():
	blocked(false),
	action(CGPathNode::ENodeAction::UNKNOWN)
{
}

void CDestinationNodeInfo::setNode(CGameState * gs, CGPathNode * n)
{
	PathNodeInfo::setNode(gs, n);

	blocked = false;
	action = CGPathNode::ENodeAction::UNKNOWN;
}

bool CDestinationNodeInfo::isBetterWay() const
{
	if(node->turns == 0xff) //we haven't been here before
		return true;
	else
		return cost < node->getCost(); //this route is faster
}

bool PathNodeInfo::isNodeObjectVisitable() const
{
	/// Hero can't visit objects while walking on water or flying
	return (node->layer == EPathfindingLayer::LAND || node->layer == EPathfindingLayer::SAIL)
		&& (canSeeObj(nodeObject) || canSeeObj(nodeHero));
}

VCMI_LIB_NAMESPACE_END
