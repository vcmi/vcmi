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
#include "../lib/CPlayerState.h"

bool canSeeObj(const CGObjectInstance * obj)
{
	/// Pathfinder should ignore placed events
	return obj != nullptr && obj->ID != Obj::EVENT;
}

std::vector<CGPathNode *> NodeStorage::calculateNeighbours(
	const PathNodeInfo & source,
	const PathfinderConfig * pathfinderConfig,
	const CPathfinderHelper * pathfinderHelper)
{
	std::vector<CGPathNode *> neighbours;
	auto accessibleNeighbourTiles = pathfinderHelper->getNeighbourTiles(source);

	for(auto & neighbour : accessibleNeighbourTiles)
	{
		for(EPathfindingLayer i = EPathfindingLayer::LAND; i <= EPathfindingLayer::AIR; i.advance(1))
		{
			auto node = getNode(neighbour, i);

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
	auto accessibleExits = pathfinderHelper->getTeleportExits(source);

	for(auto & neighbour : accessibleExits)
	{
		auto node = getNode(neighbour, source.node->layer);

		neighbours.push_back(node);
	}

	return neighbours;
}

std::vector<int3> CPathfinderHelper::getNeighbourTiles(const PathNodeInfo & source) const
{
	std::vector<int3> neighbourTiles;

	getNeighbours(
		*source.tile,
		source.node->coord,
		neighbourTiles,
		boost::logic::indeterminate,
		source.node->layer == EPathfindingLayer::SAIL);

	if(source.isNodeObjectVisitable())
	{
		vstd::erase_if(neighbourTiles, [&](int3 tile) -> bool
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
	out.hpos = hero->getPosition(false);
}

CGPathNode * NodeStorage::getNode(const int3 & coord, const EPathfindingLayer layer)
{
	return out.getNode(coord, layer);
}

void NodeStorage::resetTile(
	const int3 & tile,
	EPathfindingLayer layer,
	CGPathNode::EAccessibility accessibility)
{
	getNode(tile, layer)->update(tile, layer, accessibility);
}

CGPathNode * NodeStorage::getInitialNode()
{
	auto initialNode =  getNode(out.hpos, out.hero->boat ? EPathfindingLayer::SAIL : EPathfindingLayer::LAND);

	initialNode->turns = 0;
	initialNode->moveRemains = out.hero->movement;

	return initialNode;
}

void NodeStorage::commit(CDestinationNodeInfo & destination, const PathNodeInfo & source)
{
	assert(destination.node != source.node->theNodeBefore); //two tiles can't point to each other
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
	int turnAtNextTile = destination.turn, moveAtNextTile = destination.movementLeft;
	int cost = pathfinderHelper->getMovementCost(source, destination, moveAtNextTile);
	int remains = moveAtNextTile - cost;
	if(remains < 0)
	{
		//occurs rarely, when hero with low movepoints tries to leave the road
		pathfinderHelper->updateTurnInfo(++turnAtNextTile);
		moveAtNextTile = pathfinderHelper->getMaxMovePoints(destination.node->layer);
		cost = pathfinderHelper->getMovementCost(source, destination, moveAtNextTile); //cost must be updated, movement points changed :(
		remains = moveAtNextTile - cost;
	}
	if(destination.action == CGPathNode::EMBARK || destination.action == CGPathNode::DISEMBARK)
	{
		/// FREE_SHIP_BOARDING bonus only remove additional penalty
		/// land <-> sail transition still cost movement points as normal movement
		remains = pathfinderHelper->movementPointsAfterEmbark(moveAtNextTile, cost, destination.action - 1);
	}

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

PathfinderConfig::PathfinderConfig(
	std::shared_ptr<INodeStorage> nodeStorage,
	std::vector<std::shared_ptr<IPathfindingRule>> rules)
	: nodeStorage(nodeStorage), rules(rules), options()
{
}

CPathfinder::CPathfinder(
	CPathsInfo & _out,
	CGameState * _gs,
	const CGHeroInstance * _hero)
	: CPathfinder(
		_gs,
		_hero,
		std::make_shared<PathfinderConfig>(
			std::make_shared<NodeStorage>(_out, _hero),
			std::vector<std::shared_ptr<IPathfindingRule>>{
				std::make_shared<LayerTransitionRule>(),
				std::make_shared<DestinationActionRule>(),
				std::make_shared<MovementToDestinationRule>(),
				std::make_shared<MovementCostRule>(),
				std::make_shared<MovementAfterDestinationRule>()
			}))
{
}

CPathfinder::CPathfinder(
	CGameState * _gs,
	const CGHeroInstance * _hero,
	std::shared_ptr<PathfinderConfig> config)
	: CGameInfoCallback(_gs, boost::optional<PlayerColor>())
	, hero(_hero)
	, FoW(getPlayerTeam(hero->tempOwner)->fogOfWarMap), patrolTiles({})
	, config(config)
	, source()
	, destination()
{
	assert(hero);
	assert(hero == getHero(hero->id));

	hlp = make_unique<CPathfinderHelper>(_gs, hero, config->options);

	initializePatrol();
	initializeGraph();
}

void CPathfinder::calculatePaths()
{
	//logGlobal->info("Calculating paths for hero %s (adress  %d) of player %d", hero->name, hero , hero->tempOwner);

	//initial tile - set cost on 0 and add to the queue
	CGPathNode * initialNode = config->nodeStorage->getInitialNode();

	if(!isInTheMap(initialNode->coord)/* || !gs->map->isInTheMap(dest)*/) //check input
	{
		logGlobal->error("CGameState::calculatePaths: Hero outside the gs->map? How dare you...");
		throw std::runtime_error("Wrong checksum");
	}

	if(isHeroPatrolLocked())
		return;

	pq.push(initialNode);
	while(!pq.empty())
	{
		auto node = pq.top();
		auto excludeOurHero = node->coord == initialNode->coord;

		source.setNode(gs, node, excludeOurHero);
		pq.pop();
		source.node->locked = true;

		int movement = source.node->moveRemains, turn = source.node->turns;
		hlp->updateTurnInfo(turn);
		if(!movement)
		{
			hlp->updateTurnInfo(++turn);
			movement = hlp->getMaxMovePoints(source.node->layer);
			if(!hlp->passOneTurnLimitCheck(source))
				continue;
		}

		source.guarded = isSourceGuarded();
		if(source.nodeObject)
			source.objectRelations = gs->getPlayerRelations(hero->tempOwner, source.nodeObject->tempOwner);

		//add accessible neighbouring nodes to the queue
		auto neighbourNodes = config->nodeStorage->calculateNeighbours(source, config.get(), hlp.get());
		for(CGPathNode * neighbour : neighbourNodes)
		{
			if(neighbour->locked)
				continue;

			if(!isPatrolMovementAllowed(neighbour->coord))
				continue;

			if(!hlp->isLayerAvailable(neighbour->layer))
				continue;

			destination.setNode(gs, neighbour);

			/// Check transition without tile accessability rules
			if(source.node->layer != neighbour->layer && !isLayerTransitionPossible())
				continue;

			destination.turn = turn;
			destination.movementLeft = movement;

			destination.guarded = isDestinationGuarded();
			destination.isGuardianTile = destination.guarded && isDestinationGuardian();
			if(destination.nodeObject)
				destination.objectRelations = gs->getPlayerRelations(hero->tempOwner, destination.nodeObject->tempOwner);
				
			for(auto rule : config->rules)
			{
				rule->process(source, destination, config.get(), hlp.get());

				if(destination.blocked)
					break;
			}

			if(!destination.blocked)
				pq.push(destination.node);
			
		} //neighbours loop

		//just add all passable teleport exits

		/// For now we disable teleports usage for patrol movement
		/// VCAI not aware about patrol and may stuck while attempt to use teleport
		if(!source.isNodeObjectVisitable() || patrolState == PATROL_RADIUS)
			continue;

		auto teleportationNodes = config->nodeStorage->calculateTeleportations(source, config.get(), hlp.get());
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

			destination.setNode(gs, teleportNode);
			destination.turn = turn;
			destination.movementLeft = movement;

			if(destination.isBetterWay())
			{
				destination.action = getTeleportDestAction();
				config->nodeStorage->commit(destination, source);

				if(destination.node->action == CGPathNode::TELEPORT_NORMAL)
					pq.push(destination.node);
			}
		}
	} //queue loop
}

std::vector<int3> CPathfinderHelper::getAllowedTeleportChannelExits(TeleportChannelID channelID) const
{
	std::vector<int3> allowedExits;

	for(auto objId : getTeleportChannelExits(channelID, hero->tempOwner))
	{
		auto obj = getObj(objId);
		if(dynamic_cast<const CGWhirlpool *>(obj))
		{
			auto pos = obj->getBlockedPos();
			for(auto p : pos)
			{
				if(gs->map->getTile(p).topVisitableId() == obj->ID)
					allowedExits.push_back(p);
			}
		}
		else if(CGTeleport::isExitPassable(gs, hero, obj))
			allowedExits.push_back(obj->visitablePos());
	}

	return allowedExits;
}

std::vector<int3> CPathfinderHelper::getCastleGates(const PathNodeInfo & source) const
{
	std::vector<int3> allowedExits;

	auto towns = getPlayer(hero->tempOwner)->towns;
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

	const CGTeleport * objTeleport = dynamic_cast<const CGTeleport *>(source.nodeObject);
	if(isAllowedTeleportEntrance(objTeleport))
	{
		for(auto exit : getAllowedTeleportChannelExits(objTeleport->channel))
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
		for(auto exit : getCastleGates(source))
		{
			teleportationExits.push_back(exit);
		}
	}

	return teleportationExits;
}

bool CPathfinder::isHeroPatrolLocked() const
{
	return patrolState == PATROL_LOCKED;
}

bool CPathfinder::isPatrolMovementAllowed(const int3 & dst) const
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
			if(!config->options.lightweightFlyingMode || isSourceInitialPosition())
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
		else if(source.node->accessible != CGPathNode::ACCESSIBLE &&	destination.node->accessible != CGPathNode::ACCESSIBLE)
		{
			/// Hero that fly can only land on accessible tiles
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

			if(destination.nodeObject->ID != Obj::BOAT && destination.nodeObject->ID != Obj::HERO)
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
		const CGTeleport * objTeleport = dynamic_cast<const CGTeleport *>(destination.nodeObject);
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
		return;
	}

	CGPathNode::ENodeAction action = CGPathNode::NORMAL;
	auto hero = pathfinderHelper->hero;

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
			else if(destination.nodeObject->ID == Obj::HERO)
			{
				if(objRel == PlayerRelations::ENEMIES)
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
				if(pathfinderConfig->options.originalMovementRules && destination.guarded)
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
	if(destination.isNodeObjectVisitable() && destination.nodeObject->ID == Obj::HERO)
	{
		auto objRel = getPlayerRelations(destination.nodeObject->tempOwner, hero->tempOwner);
		if(objRel == PlayerRelations::ENEMIES)
			action = CGPathNode::TELEPORT_BATTLE;
		else
			action = CGPathNode::TELEPORT_BLOCKING_VISIT;
	}

	return action;
}

bool CPathfinder::isSourceInitialPosition() const
{
	return source.node->coord == config->nodeStorage->getInitialNode()->coord;
}

bool CPathfinder::isSourceGuarded() const
{
	/// Hero can move from guarded tile if movement started on that tile
	/// It's possible at least in these cases:
	/// - Map start with hero on guarded tile
	/// - Dimention door used
	/// TODO: check what happen when there is several guards
	if(gs->guardingCreaturePosition(source.node->coord).valid() && !isSourceInitialPosition())
	{
		return true;
	}

	return false;
}

bool CPathfinder::isDestinationGuarded() const
{
	/// isDestinationGuarded is exception needed for garrisons.
	/// When monster standing behind garrison it's visitable and guarded at the same time.
	return gs->guardingCreaturePosition(destination.node->coord).valid();
}

bool CPathfinder::isDestinationGuardian() const
{
	return gs->guardingCreaturePosition(source.node->coord) == destination.node->coord;
}

void CPathfinder::initializePatrol()
{
	auto state = PATROL_NONE;
	if(hero->patrol.patrolling && !getPlayer(hero->tempOwner)->human)
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
	auto updateNode = [&](int3 pos, ELayer layer, const TerrainTile * tinfo)
	{
		auto accessibility = evaluateAccessibility(pos, tinfo, layer);
		
		config->nodeStorage->resetTile(pos, layer, accessibility);
	};

	int3 pos;
	int3 sizes = gs->getMapSize();
	for(pos.x=0; pos.x < sizes.x; ++pos.x)
	{
		for(pos.y=0; pos.y < sizes.y; ++pos.y)
		{
			for(pos.z=0; pos.z < sizes.z; ++pos.z)
			{
				const TerrainTile * tinfo = &gs->map->getTile(pos);
				switch(tinfo->terType)
				{
				case ETerrainType::ROCK:
					break;

				case ETerrainType::WATER:
					updateNode(pos, ELayer::SAIL, tinfo);
					if(config->options.useFlying)
						updateNode(pos, ELayer::AIR, tinfo);
					if(config->options.useWaterWalking)
						updateNode(pos, ELayer::WATER, tinfo);
					break;

				default:
					updateNode(pos, ELayer::LAND, tinfo);
					if(config->options.useFlying)
						updateNode(pos, ELayer::AIR, tinfo);
					break;
				}
			}
		}
	}
}

CGPathNode::EAccessibility CPathfinder::evaluateAccessibility(const int3 & pos, const TerrainTile * tinfo, const ELayer layer) const
{
	if(tinfo->terType == ETerrainType::ROCK || !FoW[pos.x][pos.y][pos.z])
		return CGPathNode::BLOCKED;

	switch(layer)
	{
	case ELayer::LAND:
	case ELayer::SAIL:
		if(tinfo->visitable)
		{
			if(tinfo->visitableObjects.front()->ID == Obj::SANCTUARY && tinfo->visitableObjects.back()->ID == Obj::HERO && tinfo->visitableObjects.back()->tempOwner != hero->tempOwner) //non-owned hero stands on Sanctuary
			{
				return CGPathNode::BLOCKED;
			}
			else
			{
				for(const CGObjectInstance * obj : tinfo->visitableObjects)
				{
					if(obj->blockVisit)
					{
						return CGPathNode::BLOCKVIS;
					}
					else if(obj->passableFor(hero->tempOwner))
					{
						return CGPathNode::ACCESSIBLE;
					}
					else if(canSeeObj(obj))
					{
						return CGPathNode::VISITABLE;
					}
				}
			}
		}
		else if(tinfo->blocked)
		{
			return CGPathNode::BLOCKED;
		}
		else if(gs->guardingCreaturePosition(pos).valid())
		{
			// Monster close by; blocked visit for battle
			return CGPathNode::BLOCKVIS;
		}

		break;

	case ELayer::WATER:
		if(tinfo->blocked || tinfo->terType != ETerrainType::WATER)
			return CGPathNode::BLOCKED;

		break;

	case ELayer::AIR:
		if(tinfo->blocked || tinfo->terType == ETerrainType::WATER)
			return CGPathNode::FLYABLE;

		break;
	}

	return CGPathNode::ACCESSIBLE;
}

bool CPathfinderHelper::canMoveBetween(const int3 & a, const int3 & b) const
{
	return gs->checkForVisitableDir(a, b);
}

bool CPathfinderHelper::isAllowedTeleportEntrance(const CGTeleport * obj) const
{
	if(!obj || !isTeleportEntrancePassable(obj, hero->tempOwner))
		return false;

	auto whirlpool = dynamic_cast<const CGWhirlpool *>(obj);
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

int CPathfinderHelper::getHeroMaxMovementPoints(EPathfindingLayer layer) const
{
	return hero->maxMovePoints(layer);
}

int CPathfinderHelper::movementPointsAfterEmbark(int movement, int turn, int action) const
{
	return hero->movementPointsAfterEmbark(movement, turn, action, getTurnInfo());
}

bool CPathfinderHelper::passOneTurnLimitCheck(const PathNodeInfo & source) const
{

	if(!options.oneTurnSpecialLayersLimit)
		return true;

	if(source.node->layer == EPathfindingLayer::WATER)
		return false;
	if(source.node->layer == EPathfindingLayer::AIR)
	{
		if(options.originalMovementRules && source.node->accessible == CGPathNode::ACCESSIBLE)
			return true;
		else
			return false;
	}

	return true;
}

TurnInfo::BonusCache::BonusCache(TBonusListPtr bl)
{
	noTerrainPenalty.reserve(ETerrainType::ROCK);
	for(int i = 0; i < ETerrainType::ROCK; i++)
	{
		noTerrainPenalty.push_back(static_cast<bool>(
				bl->getFirst(Selector::type(Bonus::NO_TERRAIN_PENALTY).And(Selector::subtype(i)))));
	}

	freeShipBoarding = static_cast<bool>(bl->getFirst(Selector::type(Bonus::FREE_SHIP_BOARDING)));
	flyingMovement = static_cast<bool>(bl->getFirst(Selector::type(Bonus::FLYING_MOVEMENT)));
	flyingMovementVal = bl->valOfBonuses(Selector::type(Bonus::FLYING_MOVEMENT));
	waterWalking = static_cast<bool>(bl->getFirst(Selector::type(Bonus::WATER_WALKING)));
	waterWalkingVal = bl->valOfBonuses(Selector::type(Bonus::WATER_WALKING));
}

TurnInfo::TurnInfo(const CGHeroInstance * Hero, const int turn)
	: hero(Hero), maxMovePointsLand(-1), maxMovePointsWater(-1)
{
	std::stringstream cachingStr;
	cachingStr << "days_" << turn;

	bonuses = hero->getAllBonuses(Selector::days(turn), nullptr, nullptr, cachingStr.str());
	bonusCache = make_unique<BonusCache>(bonuses);
	nativeTerrain = hero->getNativeTerrain();
}

bool TurnInfo::isLayerAvailable(const EPathfindingLayer layer) const
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
			bonuses->getFirst(Selector::type(type).And(Selector::subtype(subtype))));
}

int TurnInfo::valOfBonuses(Bonus::BonusType type, int subtype) const
{
	switch(type)
	{
	case Bonus::FLYING_MOVEMENT:
		return bonusCache->flyingMovementVal;
	case Bonus::WATER_WALKING:
		return bonusCache->waterWalkingVal;
	}

	return bonuses->valOfBonuses(Selector::type(type).And(Selector::subtype(subtype)));
}

int TurnInfo::getMaxMovePoints(const EPathfindingLayer layer) const
{
	if(maxMovePointsLand == -1)
		maxMovePointsLand = hero->maxMovePoints(true, this);
	if(maxMovePointsWater == -1)
		maxMovePointsWater = hero->maxMovePoints(false, this);

	return layer == EPathfindingLayer::SAIL ? maxMovePointsWater : maxMovePointsLand;
}

CPathfinderHelper::CPathfinderHelper(CGameState * gs, const CGHeroInstance * Hero, const PathfinderOptions & Options)
	: CGameInfoCallback(gs, boost::optional<PlayerColor>()), turn(-1), hero(Hero), options(Options)
{
	turnsInfo.reserve(16);
	updateTurnInfo();
}

CPathfinderHelper::~CPathfinderHelper()
{
	for(auto ti : turnsInfo)
		delete ti;
}

void CPathfinderHelper::updateTurnInfo(const int Turn)
{
	if(turn != Turn)
	{
		turn = Turn;
		if(turn >= turnsInfo.size())
		{
			auto ti = new TurnInfo(hero, turn);
			turnsInfo.push_back(ti);
		}
	}
}

bool CPathfinderHelper::isLayerAvailable(const EPathfindingLayer layer) const
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

int CPathfinderHelper::getMaxMovePoints(const EPathfindingLayer layer) const
{
	return turnsInfo[turn]->getMaxMovePoints(layer);
}

void CPathfinderHelper::getNeighbours(
	const TerrainTile & srct, 
	const int3 & tile,
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

	for(auto & dir : dirs)
	{
		const int3 hlp = tile + dir;
		if(!map->isInTheMap(hlp))
			continue;

		const TerrainTile & hlpt = map->getTile(hlp);
		if(hlpt.terType == ETerrainType::ROCK)
			continue;

// 		//we cannot visit things from blocked tiles
// 		if(srct.blocked && !srct.visitable && hlpt.visitable && srct.blockingObjects.front()->ID != HEROI_TYPE)
// 		{
// 			continue;
// 		}

		/// Following condition let us avoid diagonal movement over coast when sailing
		if(srct.terType == ETerrainType::WATER && limitCoastSailing && hlpt.terType == ETerrainType::WATER && dir.x && dir.y) //diagonal move through water
		{
			int3 hlp1 = tile,
				hlp2 = tile;
			hlp1.x += dir.x;
			hlp2.y += dir.y;

			if(map->getTile(hlp1).terType != ETerrainType::WATER || map->getTile(hlp2).terType != ETerrainType::WATER)
				continue;
		}

		if(indeterminate(onLand) || onLand == (hlpt.terType != ETerrainType::WATER))
		{
			vec.push_back(hlp);
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

	auto ti = getTurnInfo();

	if(ct == nullptr || dt == nullptr)
	{
		ct = hero->cb->getTile(src);
		dt = hero->cb->getTile(dst);
	}

	/// TODO: by the original game rules hero shouldn't be affected by terrain penalty while flying.
	/// Also flying movement only has penalty when player moving over blocked tiles.
	/// So if you only have base flying with 40% penalty you can still ignore terrain penalty while having zero flying penalty.
	int ret = hero->getTileCost(*dt, *ct, ti);
	/// Unfortunately this can't be implemented yet as server don't know when player flying and when he's not.
	/// Difference in cost calculation on client and server is much worse than incorrect cost.
	/// So this one is waiting till server going to use pathfinder rules for path validation.

	if(dt->blocked && ti->hasBonusOfType(Bonus::FLYING_MOVEMENT))
	{
		ret *= (100.0 + ti->valOfBonuses(Bonus::FLYING_MOVEMENT)) / 100.0;
	}
	else if(dt->terType == ETerrainType::WATER)
	{
		if(hero->boat && ct->hasFavorableWinds() && dt->hasFavorableWinds())
			ret *= 0.666;
		else if(!hero->boat && ti->hasBonusOfType(Bonus::WATER_WALKING))
		{
			ret *= (100.0 + ti->valOfBonuses(Bonus::WATER_WALKING)) / 100.0;
		}
	}

	if(src.x != dst.x && src.y != dst.y) //it's diagonal move
	{
		int old = ret;
		ret *= 1.414213;
		//diagonal move costs too much but normal move is possible - allow diagonal move for remaining move points
		if(ret > remainingMovePoints && remainingMovePoints >= old)
		{
			return remainingMovePoints;
		}
	}

	/// TODO: This part need rework in order to work properly with flying and water walking
	/// Currently it's only work properly for normal movement or sailing
	int left = remainingMovePoints-ret;
	if(checkLast && left > 0 && remainingMovePoints-ret < 250) //it might be the last tile - if no further move possible we take all move points
	{
		std::vector<int3> vec;
		vec.reserve(8); //optimization
		getNeighbours(*dt, dst, vec, ct->terType != ETerrainType::WATER, true);
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

CGPathNode::CGPathNode()
	: coord(int3(-1, -1, -1)), layer(ELayer::WRONG)
{
	reset();
}

void CGPathNode::reset()
{
	locked = false;
	accessible = NOT_SET;
	moveRemains = 0;
	turns = 255;
	theNodeBefore = nullptr;
	action = UNKNOWN;
}

void CGPathNode::update(const int3 & Coord, const ELayer Layer, const EAccessibility Accessible)
{
	if(layer == ELayer::WRONG)
	{
		coord = Coord;
		layer = Layer;
	}
	else
		reset();

	accessible = Accessible;
}

bool CGPathNode::reachable() const
{
	return turns < 255;
}

int3 CGPath::startPos() const
{
	return nodes[nodes.size()-1].coord;
}

int3 CGPath::endPos() const
{
	return nodes[0].coord;
}

void CGPath::convert(ui8 mode)
{
	if(mode==0)
	{
		for(auto & elem : nodes)
		{
			elem.coord = CGHeroInstance::convertPosition(elem.coord,true);
		}
	}
}

CPathsInfo::CPathsInfo(const int3 & Sizes)
	: sizes(Sizes)
{
	hero = nullptr;
	nodes.resize(boost::extents[sizes.x][sizes.y][sizes.z][ELayer::NUM_LAYERS]);
}

CPathsInfo::~CPathsInfo()
{
}

const CGPathNode * CPathsInfo::getPathInfo(const int3 & tile) const
{
	assert(vstd::iswithin(tile.x, 0, sizes.x));
	assert(vstd::iswithin(tile.y, 0, sizes.y));
	assert(vstd::iswithin(tile.z, 0, sizes.z));

	boost::unique_lock<boost::mutex> pathLock(pathMx);
	return getNode(tile);
}

bool CPathsInfo::getPath(CGPath & out, const int3 & dst) const
{
	boost::unique_lock<boost::mutex> pathLock(pathMx);

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

int CPathsInfo::getDistance(const int3 & tile) const
{
	boost::unique_lock<boost::mutex> pathLock(pathMx);

	CGPath ret;
	if(getPath(ret, tile))
		return ret.nodes.size();
	else
		return 255;
}

const CGPathNode * CPathsInfo::getNode(const int3 & coord) const
{
	auto landNode = &nodes[coord.x][coord.y][coord.z][ELayer::LAND];
	if(landNode->reachable())
		return landNode;
	else
		return &nodes[coord.x][coord.y][coord.z][ELayer::SAIL];
}

CGPathNode * CPathsInfo::getNode(const int3 & coord, const ELayer layer)
{
	return &nodes[coord.x][coord.y][coord.z][layer];
}

PathNodeInfo::PathNodeInfo()
	: node(nullptr), nodeObject(nullptr), tile(nullptr), coord(-1, -1, -1), guarded(false)
{
}

void PathNodeInfo::setNode(CGameState * gs, CGPathNode * n, bool excludeTopObject)
{
	node = n;

	if(coord != node->coord)
	{
		assert(node->coord.valid());

		coord = node->coord;
		tile = gs->getTile(coord);
		nodeObject = tile->topVisitableObj(excludeTopObject);
	}

	guarded = false;
}

CDestinationNodeInfo::CDestinationNodeInfo()
	: PathNodeInfo(), blocked(false), action(CGPathNode::ENodeAction::UNKNOWN)
{
}

void CDestinationNodeInfo::setNode(CGameState * gs, CGPathNode * n, bool excludeTopObject)
{
	PathNodeInfo::setNode(gs, n, excludeTopObject);

	blocked = false;
	action = CGPathNode::ENodeAction::UNKNOWN;
}

bool CDestinationNodeInfo::isBetterWay() const
{
	if(node->turns == 0xff) //we haven't been here before
		return true;
	else if(node->turns > turn)
		return true;
	else if(node->turns >= turn && node->moveRemains < movementLeft) //this route is faster
		return true;

	return false;

}

bool PathNodeInfo::isNodeObjectVisitable() const
{
	/// Hero can't visit objects while walking on water or flying
	return canSeeObj(nodeObject) && (node->layer == EPathfindingLayer::LAND || node->layer == EPathfindingLayer::SAIL);
}
