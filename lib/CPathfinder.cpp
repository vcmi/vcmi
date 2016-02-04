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

/*
 * CPathfinder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CPathfinder::PathfinderOptions::PathfinderOptions()
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

CPathfinder::CPathfinder(CPathsInfo & _out, CGameState * _gs, const CGHeroInstance * _hero)
	: CGameInfoCallback(_gs, boost::optional<PlayerColor>()), out(_out), hero(_hero), FoW(getPlayerTeam(hero->tempOwner)->fogOfWarMap), patrolTiles({})
{
	assert(hero);
	assert(hero == getHero(hero->id));

	out.hero = hero;
	out.hpos = hero->getPosition(false);
	if(!isInTheMap(out.hpos)/* || !gs->map->isInTheMap(dest)*/) //check input
	{
		logGlobal->errorStream() << "CGameState::calculatePaths: Hero outside the gs->map? How dare you...";
		throw std::runtime_error("Wrong checksum");
	}

	hlp = make_unique<CPathfinderHelper>(hero, options);

	initializePatrol();
	initializeGraph();
	neighbourTiles.reserve(8);
	neighbours.reserve(16);
}

void CPathfinder::calculatePaths()
{
	auto passOneTurnLimitCheck = [&]() -> bool
	{
		if(!options.oneTurnSpecialLayersLimit)
			return true;

		if(cp->layer == ELayer::WATER)
			return false;
		if(cp->layer == ELayer::AIR)
		{
			if(options.originalMovementRules && cp->accessible == CGPathNode::ACCESSIBLE)
				return true;
			else
				return false;
		}

		return true;
	};

	auto isBetterWay = [&](int remains, int turn) -> bool
	{
		if(dp->turns == 0xff) //we haven't been here before
			return true;
		else if(dp->turns > turn)
			return true;
		else if(dp->turns >= turn && dp->moveRemains < remains) //this route is faster
			return true;

		return false;
	};

	//logGlobal->infoStream() << boost::format("Calculating paths for hero %s (adress  %d) of player %d") % hero->name % hero % hero->tempOwner;

	//initial tile - set cost on 0 and add to the queue
	CGPathNode * initialNode = out.getNode(out.hpos, hero->boat ? ELayer::SAIL : ELayer::LAND);
	initialNode->turns = 0;
	initialNode->moveRemains = hero->movement;
	if(isHeroPatrolLocked())
		return;

	pq.push(initialNode);
	while(!pq.empty())
	{
		cp = pq.top();
		pq.pop();
		cp->locked = true;

		int movement = cp->moveRemains, turn = cp->turns;
		hlp->updateTurnInfo(turn);
		if(!movement)
		{
			hlp->updateTurnInfo(++turn);
			movement = hlp->getMaxMovePoints(cp->layer);
			if(!passOneTurnLimitCheck())
				continue;
		}
		ct = &gs->map->getTile(cp->coord);
		ctObj = ct->topVisitableObj(isSourceInitialPosition());

		//add accessible neighbouring nodes to the queue
		addNeighbours();
		for(auto & neighbour : neighbours)
		{
			if(!isPatrolMovementAllowed(neighbour))
				continue;

			dt = &gs->map->getTile(neighbour);
			dtObj = dt->topVisitableObj();
			for(ELayer i = ELayer::LAND; i <= ELayer::AIR; i.advance(1))
			{
				if(!hlp->isLayerAvailable(i))
					continue;

				if(cp->layer != i && !isLayerTransitionPossible(i))
					continue;

				dp = out.getNode(neighbour, i);
				if(dp->locked)
					continue;

				if(dp->accessible == CGPathNode::NOT_SET)
					continue;

				if(cp->layer != i && !isLayerTransitionPossible())
					continue;

				if(!isMovementToDestPossible())
					continue;

				destAction = getDestAction();
				int turnAtNextTile = turn, moveAtNextTile = movement;
				int cost = CPathfinderHelper::getMovementCost(hero, cp->coord, dp->coord, ct, dt, moveAtNextTile, hlp->getTurnInfo());
				int remains = moveAtNextTile - cost;
				if(remains < 0)
				{
					//occurs rarely, when hero with low movepoints tries to leave the road
					hlp->updateTurnInfo(++turnAtNextTile);
					moveAtNextTile = hlp->getMaxMovePoints(i);
					cost = CPathfinderHelper::getMovementCost(hero, cp->coord, dp->coord, ct, dt, moveAtNextTile, hlp->getTurnInfo()); //cost must be updated, movement points changed :(
					remains = moveAtNextTile - cost;
				}
				if(destAction == CGPathNode::EMBARK || destAction == CGPathNode::DISEMBARK)
				{
					/// FREE_SHIP_BOARDING bonus only remove additional penalty
					/// land <-> sail transition still cost movement points as normal movement
					remains = hero->movementPointsAfterEmbark(moveAtNextTile, cost, destAction - 1, hlp->getTurnInfo());
					cost = moveAtNextTile - remains;
				}

				if(isBetterWay(remains, turnAtNextTile) &&
					((cp->turns == turnAtNextTile && remains) || passOneTurnLimitCheck()))
				{
					assert(dp != cp->theNodeBefore); //two tiles can't point to each other
					dp->moveRemains = remains;
					dp->turns = turnAtNextTile;
					dp->theNodeBefore = cp;
					dp->action = destAction;

					if(isMovementAfterDestPossible())
						pq.push(dp);
				}
			}
		} //neighbours loop

		//just add all passable teleport exits
		addTeleportExits();
		for(auto & neighbour : neighbours)
		{
			dp = out.getNode(neighbour, cp->layer);
			if(dp->locked)
				continue;
			/// TODO: We may consider use invisible exits on FoW border in future
			/// Useful for AI when at least one tile around exit is visible and passable
			/// Objects are usually visible on FoW border anyway so it's not cheating.
			///
			/// For now it's disabled as it's will cause crashes in movement code.
			if(dp->accessible == CGPathNode::BLOCKED)
				continue;

			if(isBetterWay(movement, turn))
			{
				dtObj = gs->map->getTile(neighbour).topVisitableObj();

				dp->moveRemains = movement;
				dp->turns = turn;
				dp->theNodeBefore = cp;
				dp->action = getTeleportDestAction();
				if(dp->action == CGPathNode::TELEPORT_NORMAL)
					pq.push(dp);
			}
		}
	} //queue loop
}

void CPathfinder::addNeighbours()
{
	neighbours.clear();
	neighbourTiles.clear();
	CPathfinderHelper::getNeighbours(gs->map, *ct, cp->coord, neighbourTiles, boost::logic::indeterminate, cp->layer == ELayer::SAIL);
	if(isSourceVisitableObj())
	{
		for(int3 tile: neighbourTiles)
		{
			if(canMoveBetween(tile, ctObj->visitablePos()))
				neighbours.push_back(tile);
		}
	}
	else
		vstd::concatenate(neighbours, neighbourTiles);
}

void CPathfinder::addTeleportExits()
{
	neighbours.clear();
	/// For now we disable teleports usage for patrol movement
	/// VCAI not aware about patrol and may stuck while attempt to use teleport
	if(!isSourceVisitableObj() || patrolState == PATROL_RADIUS)
		return;

	const CGTeleport * objTeleport = dynamic_cast<const CGTeleport *>(ctObj);
	if(isAllowedTeleportEntrance(objTeleport))
	{
		for(auto objId : getTeleportChannelExits(objTeleport->channel, hero->tempOwner))
		{
			auto obj = getObj(objId);
			if(dynamic_cast<const CGWhirlpool *>(obj))
			{
				auto pos = obj->getBlockedPos();
				for(auto p : pos)
				{
					if(gs->map->getTile(p).topVisitableId() == obj->ID)
						neighbours.push_back(p);
				}
			}
			else if(CGTeleport::isExitPassable(gs, hero, obj))
				neighbours.push_back(obj->visitablePos());
		}
	}

	if(options.useCastleGate
		&& (ctObj->ID == Obj::TOWN && ctObj->subID == ETownType::INFERNO
		&& getPlayerRelations(hero->tempOwner, ctObj->tempOwner) != PlayerRelations::ENEMIES))
	{
		/// TODO: Find way to reuse CPlayerSpecificInfoCallback::getTownsInfo
		/// This may be handy if we allow to use teleportation to friendly towns
		auto towns = getPlayer(hero->tempOwner)->towns;
		for(const auto & town : towns)
		{
			if(town->id != ctObj->id && town->visitingHero == nullptr
				&& town->hasBuilt(BuildingID::CASTLE_GATE, ETownType::INFERNO))
			{
				neighbours.push_back(town->visitablePos());
			}
		}
	}
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

bool CPathfinder::isLayerTransitionPossible(const ELayer destLayer) const
{
	/// No layer transition allowed when previous node action is BATTLE
	if(cp->action == CGPathNode::BATTLE)
		return false;

	switch(cp->layer)
	{
	case ELayer::LAND:
		if(destLayer != ELayer::AIR)
			return true;

		if(!options.lightweightFlyingMode || isSourceInitialPosition())
			return true;

		break;

	case ELayer::SAIL:
		if(destLayer == ELayer::LAND && dt->isCoastal())
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

bool CPathfinder::isLayerTransitionPossible() const
{
	switch(cp->layer)
	{
	case ELayer::LAND:
		if(dp->layer == ELayer::SAIL)
		{
			/// Cannot enter empty water tile from land -> it has to be visitable
			if(dp->accessible == CGPathNode::ACCESSIBLE)
				return false;
		}

		break;

	case ELayer::SAIL:
		//tile must be accessible -> exception: unblocked blockvis tiles -> clear but guarded by nearby monster coast
		if((dp->accessible != CGPathNode::ACCESSIBLE && (dp->accessible != CGPathNode::BLOCKVIS || dt->blocked))
			|| dt->visitable)  //TODO: passableness problem -> town says it's passable (thus accessible) but we obviously can't disembark onto town gate
		{
			return false;
		}

		break;

	case ELayer::AIR:
		if(options.originalMovementRules)
		{
			if((cp->accessible != CGPathNode::ACCESSIBLE &&
				cp->accessible != CGPathNode::VISITABLE) &&
				(dp->accessible != CGPathNode::VISITABLE &&
				 dp->accessible != CGPathNode::ACCESSIBLE))
			{
				return false;
			}
		}
		else if(cp->accessible != CGPathNode::ACCESSIBLE &&	dp->accessible != CGPathNode::ACCESSIBLE)
		{
			/// Hero that fly can only land on accessible tiles
			return false;
		}

		break;

	case ELayer::WATER:
		if(dp->accessible != CGPathNode::ACCESSIBLE && dp->accessible != CGPathNode::VISITABLE)
		{
			/// Hero that walking on water can transit to accessible and visitable tiles
			/// Though hero can't interact with blocking visit objects while standing on water
			return false;
		}

		break;
	}

	return true;
}

bool CPathfinder::isMovementToDestPossible() const
{
	if(dp->accessible == CGPathNode::BLOCKED)
		return false;

	switch(dp->layer)
	{
	case ELayer::LAND:
		if(!canMoveBetween(cp->coord, dp->coord))
			return false;
		if(isSourceGuarded())
		{
			if(!(options.originalMovementRules && cp->layer == ELayer::AIR) &&
				!isDestinationGuardian()) // Can step into tile of guard
			{
				return false;
			}
		}

		break;

	case ELayer::SAIL:
		if(!canMoveBetween(cp->coord, dp->coord))
			return false;
		if(isSourceGuarded())
		{
			// Hero embarked a boat standing on a guarded tile -> we must allow to move away from that tile
			if(cp->action != CGPathNode::EMBARK && !isDestinationGuardian())
				return false;
		}

		if(cp->layer == ELayer::LAND)
		{
			if(!isDestVisitableObj())
				return false;

			if(dtObj->ID != Obj::BOAT && dtObj->ID != Obj::HERO)
				return false;
		}
		else if(isDestVisitableObj() && dtObj->ID == Obj::BOAT)
		{
			/// Hero in boat can't visit empty boats
			return false;
		}

		break;

	case ELayer::WATER:
		if(!canMoveBetween(cp->coord, dp->coord) || dp->accessible != CGPathNode::ACCESSIBLE)
			return false;
		if(isDestinationGuarded())
			return false;

		break;
	}

	return true;
}

bool CPathfinder::isMovementAfterDestPossible() const
{
	switch(destAction)
	{
	/// TODO: Investigate what kind of limitation is possible to apply on movement from visitable tiles
	/// Likely in many cases we don't need to add visitable tile to queue when hero don't fly
	case CGPathNode::VISIT:
	{
		/// For now we only add visitable tile into queue when it's teleporter that allow transit
		/// Movement from visitable tile when hero is standing on it is possible into any layer
		const CGTeleport * objTeleport = dynamic_cast<const CGTeleport *>(dtObj);
		if(isAllowedTeleportEntrance(objTeleport))
		{
			/// For now we'll always allow transit over teleporters
			/// Transit over whirlpools only allowed when hero protected
			return true;
		}
		else if(dtObj->ID == Obj::GARRISON || dtObj->ID == Obj::GARRISON2 || dtObj->ID == Obj::BORDER_GATE)
		{
			/// Transit via unguarded garrisons is always possible
			return true;
		}

		break;
	}

	case CGPathNode::NORMAL:
		return true;

	case CGPathNode::EMBARK:
		if(options.useEmbarkAndDisembark)
			return true;

		break;

	case CGPathNode::DISEMBARK:
		if(options.useEmbarkAndDisembark && !isDestinationGuarded())
			return true;

		break;

	case CGPathNode::BATTLE:
		/// Movement after BATTLE action only possible from guarded tile to guardian tile
		if(isDestinationGuarded())
			return true;

		break;
	}

	return false;
}

CGPathNode::ENodeAction CPathfinder::getDestAction() const
{
	CGPathNode::ENodeAction action = CGPathNode::NORMAL;
	switch(dp->layer)
	{
	case ELayer::LAND:
		if(cp->layer == ELayer::SAIL)
		{
			// TODO: Handle dismebark into guarded areaa
			action = CGPathNode::DISEMBARK;
			break;
		}

		/// don't break - next case shared for both land and sail layers

	case ELayer::SAIL:
		if(isDestVisitableObj())
		{
			auto objRel = getPlayerRelations(dtObj->tempOwner, hero->tempOwner);

			if(dtObj->ID == Obj::BOAT)
				action = CGPathNode::EMBARK;
			else if(dtObj->ID == Obj::HERO)
			{
				if(objRel == PlayerRelations::ENEMIES)
					action = CGPathNode::BATTLE;
				else
					action = CGPathNode::BLOCKING_VISIT;
			}
			else if(dtObj->ID == Obj::TOWN)
			{
				if(dtObj->passableFor(hero->tempOwner))
					action = CGPathNode::VISIT;
				else if(objRel == PlayerRelations::ENEMIES)
					action = CGPathNode::BATTLE;
			}
			else if(dtObj->ID == Obj::GARRISON || dtObj->ID == Obj::GARRISON2)
			{
				if(dtObj->passableFor(hero->tempOwner))
				{
					if(isDestinationGuarded(true))
						action = CGPathNode::BATTLE;
				}
				else if(objRel == PlayerRelations::ENEMIES)
					action = CGPathNode::BATTLE;
			}
			else if(dtObj->ID == Obj::BORDER_GATE)
			{
				if(dtObj->passableFor(hero->tempOwner))
				{
					if(isDestinationGuarded(true))
						action = CGPathNode::BATTLE;
				}
				else
					action = CGPathNode::BLOCKING_VISIT;
			}
			else if(isDestinationGuardian())
				action = CGPathNode::BATTLE;
			else if(dtObj->blockVisit && !(options.useCastleGate && dtObj->ID == Obj::TOWN))
				action = CGPathNode::BLOCKING_VISIT;

			if(action == CGPathNode::NORMAL)
			{
				if(options.originalMovementRules && isDestinationGuarded())
					action = CGPathNode::BATTLE;
				else
					action = CGPathNode::VISIT;
			}
		}
		else if(isDestinationGuarded())
			action = CGPathNode::BATTLE;

		break;
	}

	return action;
}

CGPathNode::ENodeAction CPathfinder::getTeleportDestAction() const
{
	CGPathNode::ENodeAction action = CGPathNode::TELEPORT_NORMAL;
	if(isDestVisitableObj() && dtObj->ID == Obj::HERO)
	{
		auto objRel = getPlayerRelations(dtObj->tempOwner, hero->tempOwner);
		if(objRel == PlayerRelations::ENEMIES)
			action = CGPathNode::TELEPORT_BATTLE;
		else
			action = CGPathNode::TELEPORT_BLOCKING_VISIT;
	}

	return action;
}

bool CPathfinder::isSourceInitialPosition() const
{
	return cp->coord == out.hpos;
}

bool CPathfinder::isSourceVisitableObj() const
{
	return isVisitableObj(ctObj, cp->layer);
}

bool CPathfinder::isSourceGuarded() const
{
	/// Hero can move from guarded tile if movement started on that tile
	/// It's possible at least in these cases:
	/// - Map start with hero on guarded tile
	/// - Dimention door used
	/// TODO: check what happen when there is several guards
	if(gs->guardingCreaturePosition(cp->coord).valid() && !isSourceInitialPosition())
	{
		return true;
	}

	return false;
}

bool CPathfinder::isDestVisitableObj() const
{
	return isVisitableObj(dtObj, dp->layer);
}

bool CPathfinder::isDestinationGuarded(const bool ignoreAccessibility) const
{
	/// isDestinationGuarded is exception needed for garrisons.
	/// When monster standing behind garrison it's visitable and guarded at the same time.
	if(gs->guardingCreaturePosition(dp->coord).valid()
		&& (ignoreAccessibility || dp->accessible == CGPathNode::BLOCKVIS))
	{
		return true;
	}

	return false;
}

bool CPathfinder::isDestinationGuardian() const
{
	return gs->guardingCreaturePosition(cp->coord) == dp->coord;
}

void CPathfinder::initializePatrol()
{
	auto state = PATROL_NONE;
	if(hero->patrol.patrolling && !getPlayer(hero->tempOwner)->human)
	{
		if(hero->patrol.patrolRadius)
		{
			state = PATROL_RADIUS;
			gs->getTilesInRange(patrolTiles, hero->patrol.initialPos, hero->patrol.patrolRadius, boost::optional<PlayerColor>(), 0, true);
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
		auto node = out.getNode(pos, layer);
		auto accessibility = evaluateAccessibility(pos, tinfo, layer);
		node->update(pos, layer, accessibility);
	};

	int3 pos;
	for(pos.x=0; pos.x < out.sizes.x; ++pos.x)
	{
		for(pos.y=0; pos.y < out.sizes.y; ++pos.y)
		{
			for(pos.z=0; pos.z < out.sizes.z; ++pos.z)
			{
				const TerrainTile * tinfo = &gs->map->getTile(pos);
				switch(tinfo->terType)
				{
				case ETerrainType::ROCK:
					break;

				case ETerrainType::WATER:
					updateNode(pos, ELayer::SAIL, tinfo);
					if(options.useFlying)
						updateNode(pos, ELayer::AIR, tinfo);
					if(options.useWaterWalking)
						updateNode(pos, ELayer::WATER, tinfo);
					break;

				default:
					updateNode(pos, ELayer::LAND, tinfo);
					if(options.useFlying)
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

bool CPathfinder::isVisitableObj(const CGObjectInstance * obj, const ELayer layer) const
{
	/// Hero can't visit objects while walking on water or flying
	return canSeeObj(obj) && (layer == ELayer::LAND || layer == ELayer::SAIL);
}

bool CPathfinder::canSeeObj(const CGObjectInstance * obj) const
{
	/// Pathfinder should ignore placed events
	return obj != nullptr && obj->ID != Obj::EVENT;
}

bool CPathfinder::canMoveBetween(const int3 & a, const int3 & b) const
{
	return gs->checkForVisitableDir(a, b);
}

bool CPathfinder::isAllowedTeleportEntrance(const CGTeleport * obj) const
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

bool CPathfinder::addTeleportTwoWay(const CGTeleport * obj) const
{
	return options.useTeleportTwoWay && isTeleportChannelBidirectional(obj->channel, hero->tempOwner);
}

bool CPathfinder::addTeleportOneWay(const CGTeleport * obj) const
{
	if(options.useTeleportOneWay && isTeleportChannelUnidirectional(obj->channel, hero->tempOwner))
	{
		auto passableExits = CGTeleport::getPassableExits(gs, hero, getTeleportChannelExits(obj->channel, hero->tempOwner));
		if(passableExits.size() == 1)
			return true;
	}
	return false;
}

bool CPathfinder::addTeleportOneWayRandom(const CGTeleport * obj) const
{
	if(options.useTeleportOneWayRandom && isTeleportChannelUnidirectional(obj->channel, hero->tempOwner))
	{
		auto passableExits = CGTeleport::getPassableExits(gs, hero, getTeleportChannelExits(obj->channel, hero->tempOwner));
		if(passableExits.size() > 1)
			return true;
	}
	return false;
}

bool CPathfinder::addTeleportWhirlpool(const CGWhirlpool * obj) const
{
	return options.useTeleportWhirlpool && hlp->hasBonusOfType(Bonus::WHIRLPOOL_PROTECTION) && obj;
}

TurnInfo::BonusCache::BonusCache(TBonusListPtr bl)
{
	noTerrainPenalty.reserve(ETerrainType::ROCK);
	for(int i = 0; i < ETerrainType::ROCK; i++)
	{
		noTerrainPenalty.push_back(bl->getFirst(Selector::type(Bonus::NO_TERRAIN_PENALTY).And(Selector::subtype(i))));
	}

	freeShipBoarding = bl->getFirst(Selector::type(Bonus::FREE_SHIP_BOARDING));
	flyingMovement = bl->getFirst(Selector::type(Bonus::FLYING_MOVEMENT));
	flyingMovementVal = bl->valOfBonuses(Selector::type(Bonus::FLYING_MOVEMENT));
	waterWalking = bl->getFirst(Selector::type(Bonus::WATER_WALKING));
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

	return bonuses->getFirst(Selector::type(type).And(Selector::subtype(subtype)));
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

CPathfinderHelper::CPathfinderHelper(const CGHeroInstance * Hero, const CPathfinder::PathfinderOptions & Options)
	: turn(-1), hero(Hero), options(Options)
{
	turnsInfo.reserve(16);
	updateTurnInfo();
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

void CPathfinderHelper::getNeighbours(const CMap * map, const TerrainTile & srct, const int3 & tile, std::vector<int3> & vec, const boost::logic::tribool & onLand, const bool limitCoastSailing)
{
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

int CPathfinderHelper::getMovementCost(const CGHeroInstance * h, const int3 & src, const int3 & dst, const TerrainTile * ct, const TerrainTile * dt, const int remainingMovePoints, const TurnInfo * ti, const bool checkLast)
{
	if(src == dst) //same tile
		return 0;

	if(!ti)
		ti = new TurnInfo(h);

	if(ct == nullptr || dt == nullptr)
	{
		ct = h->cb->getTile(src);
		dt = h->cb->getTile(dst);
	}

	/// TODO: by the original game rules hero shouldn't be affected by terrain penalty while flying.
	/// Also flying movement only has penalty when player moving over blocked tiles.
	/// So if you only have base flying with 40% penalty you can still ignore terrain penalty while having zero flying penalty.
	int ret = h->getTileCost(*dt, *ct, ti);
	/// Unfortunately this can't be implemented yet as server don't know when player flying and when he's not.
	/// Difference in cost calculation on client and server is much worse than incorrect cost.
	/// So this one is waiting till server going to use pathfinder rules for path validation.

	if(dt->blocked && ti->hasBonusOfType(Bonus::FLYING_MOVEMENT))
	{
		ret *= (100.0 + ti->valOfBonuses(Bonus::FLYING_MOVEMENT)) / 100.0;
	}
	else if(dt->terType == ETerrainType::WATER)
	{
		if(h->boat && ct->hasFavorableWinds() && dt->hasFavorableWinds())
			ret *= 0.666;
		else if(!h->boat && ti->hasBonusOfType(Bonus::WATER_WALKING))
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
			return remainingMovePoints;
	}

	/// TODO: This part need rework in order to work properly with flying and water walking
	/// Currently it's only work properly for normal movement or sailing
	int left = remainingMovePoints-ret;
	if(checkLast && left > 0 && remainingMovePoints-ret < 250) //it might be the last tile - if no further move possible we take all move points
	{
		std::vector<int3> vec;
		vec.reserve(8); //optimization
		getNeighbours(h->cb->gameState()->map, *dt, dst, vec, ct->terType != ETerrainType::WATER, true);
		for(auto & elem : vec)
		{
			int fcost = getMovementCost(h, dst, elem, nullptr, nullptr, left, ti, false);
			if(fcost <= left)
				return ret;
		}
		ret = remainingMovePoints;
	}
	return ret;
}

int CPathfinderHelper::getMovementCost(const CGHeroInstance * h, const int3 & dst)
{
	return getMovementCost(h, h->visitablePos(), dst, nullptr, nullptr, h->movement);
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
