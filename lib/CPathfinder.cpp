#include "StdInc.h"
#include "CPathfinder.h"

#include "CHeroHandler.h"
#include "mapping/CMap.h"
#include "CGameState.h"
#include "mapObjects/CGHeroInstance.h"
#include "GameConstants.h"
#include "CStopWatch.h"

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
	useFlying = false;
	useWaterWalking = false;
	useEmbarkAndDisembark = true;
	useTeleportTwoWay = true;
	useTeleportOneWay = true;
	useTeleportOneWayRandom = false;
	useTeleportWhirlpool = false;

	useCastleGate = false;

	lightweightFlyingMode = false;
	oneTurnSpecialLayersLimit = true;
	originalMovementRules = true;
}

CPathfinder::CPathfinder(CPathsInfo &_out, CGameState *_gs, const CGHeroInstance *_hero)
	: CGameInfoCallback(_gs, boost::optional<PlayerColor>()), out(_out), hero(_hero)
{
	assert(hero);
	assert(hero == getHero(hero->id));

	out.hero = hero;
	out.hpos = hero->getPosition(false);
	if(!gs->map->isInTheMap(out.hpos)/* || !gs->map->isInTheMap(dest)*/) //check input
	{
		logGlobal->errorStream() << "CGameState::calculatePaths: Hero outside the gs->map? How dare you...";
		throw std::runtime_error("Wrong checksum");
	}

	hlp = make_unique<CPathfinderHelper>(hero);
	if(hlp->ti->bonusFlying)
		options.useFlying = true;
	if(hlp->ti->bonusWaterWalking)
		options.useWaterWalking = true;
	if(CGWhirlpool::isProtected(hero))
		options.useTeleportWhirlpool = true;

	initializeGraph();
	neighbours.reserve(16);
}

void CPathfinder::calculatePaths()
{
	auto passOneTurnLimitCheck = [&](bool shouldCheck) -> bool
	{
		if(options.oneTurnSpecialLayersLimit && shouldCheck)
		{
			if((cp->layer == ELayer::AIR || cp->layer == ELayer::WATER)
				&& cp->accessible != CGPathNode::ACCESSIBLE)
			{
				return false;
			}
		}
		return true;
	};

	auto isBetterWay = [&](int remains, int turn) -> bool
	{
		if(dp->turns == 0xff) //we haven't been here before
			return true;
		else if(dp->turns > turn)
			return true;
		else if(dp->turns >= turn  &&  dp->moveRemains < remains) //this route is faster
			return true;

		return false;
	};

	//logGlobal->infoStream() << boost::format("Calculating paths for hero %s (adress  %d) of player %d") % hero->name % hero % hero->tempOwner;

	//initial tile - set cost on 0 and add to the queue
	CGPathNode *initialNode = out.getNode(out.hpos, hero->boat ? ELayer::SAIL : ELayer::LAND);
	initialNode->turns = 0;
	initialNode->moveRemains = hero->movement;
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
		}

		//add accessible neighbouring nodes to the queue
		addNeighbours(cp->coord);
		for(auto & neighbour : neighbours)
		{
			dt = &gs->map->getTile(neighbour);
			for(ELayer i = ELayer::LAND; i <= ELayer::AIR; i.advance(1))
			{
				dp = out.getNode(neighbour, i);
				if(dp->accessible == CGPathNode::NOT_SET)
					continue;

				if(dp->locked)
					continue;

				if(!passOneTurnLimitCheck(cp->turns != turn))
					continue;

				if(!isLayerAvailable(i, turn))
					continue;

				if(cp->layer != i && !isLayerTransitionPossible())
					continue;


				destAction = CGPathNode::UNKNOWN;
				if(!isMovementToDestPossible())
					continue;

				int cost = CPathfinderHelper::getMovementCost(hero, cp->coord, dp->coord, movement, hlp->ti);
				int remains = movement - cost;
				if(destAction == CGPathNode::EMBARK || destAction == CGPathNode::DISEMBARK)
				{
					remains = hero->movementPointsAfterEmbark(movement, cost, destAction - 1);
					cost = movement - remains;
				}
				int turnAtNextTile = turn;
				if(remains < 0)
				{
					//occurs rarely, when hero with low movepoints tries to leave the road
					hlp->updateTurnInfo(++turnAtNextTile);
					int moveAtNextTile = hlp->getMaxMovePoints(i);
					cost = CPathfinderHelper::getMovementCost(hero, cp->coord, dp->coord, moveAtNextTile, hlp->ti); //cost must be updated, movement points changed :(
					remains = moveAtNextTile - cost;
				}

				if(isBetterWay(remains, turnAtNextTile)
					&& passOneTurnLimitCheck(cp->turns != turnAtNextTile || !remains))
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
		if(sTileObj && canVisitObject())
		{
			addTeleportExits();
			for(auto & neighbour : neighbours)
			{
				dp = out.getNode(neighbour, cp->layer);
				if(dp->locked)
					continue;

				if(isBetterWay(movement, turn))
				{
					dp->moveRemains = movement;
					dp->turns = turn;
					dp->theNodeBefore = cp;
					dp->action = CGPathNode::NORMAL;
					pq.push(dp);
				}
			}
		}
	} //queue loop
}

void CPathfinder::addNeighbours(const int3 &coord)
{
	neighbours.clear();
	ct = &gs->map->getTile(coord);

	std::vector<int3> tiles;
	CPathfinderHelper::getNeighbours(gs, *ct, coord, tiles, boost::logic::indeterminate, cp->layer == ELayer::SAIL); // TODO: find out if we still need "limitCoastSailing" option
	sTileObj = ct->topVisitableObj(coord == out.hpos);
	if(canVisitObject())
	{
		if(sTileObj)
		{
			for(int3 tile: tiles)
			{
				if(canMoveBetween(tile, sTileObj->visitablePos()))
					neighbours.push_back(tile);
			}
		}
		else
			vstd::concatenate(neighbours, tiles);
	}
	else
		vstd::concatenate(neighbours, tiles);
}

void CPathfinder::addTeleportExits(bool noTeleportExcludes)
{
	assert(sTileObj);

	neighbours.clear();
	auto isAllowedTeleportEntrance = [&](const CGTeleport * obj) -> bool
	{
		if(!gs->isTeleportEntrancePassable(obj, hero->tempOwner))
			return false;

		if(noTeleportExcludes)
			return true;

		auto whirlpool = dynamic_cast<const CGWhirlpool *>(obj);
		if(whirlpool)
		{
			if(addTeleportWhirlpool(whirlpool))
				return true;
		}
		else if(addTeleportTwoWay(obj) || addTeleportOneWay(obj) || addTeleportOneWayRandom(obj))
			return true;

		return false;
	};

	const CGTeleport *sTileTeleport = dynamic_cast<const CGTeleport *>(sTileObj);
	if(isAllowedTeleportEntrance(sTileTeleport))
	{
		for(auto objId : gs->getTeleportChannelExits(sTileTeleport->channel, hero->tempOwner))
		{
			auto obj = getObj(objId);
			if(CGTeleport::isExitPassable(gs, hero, obj))
				neighbours.push_back(obj->visitablePos());
		}
	}

	if(options.useCastleGate
		&& (sTileObj->ID == Obj::TOWN && sTileObj->subID == ETownType::INFERNO
		&& getPlayerRelations(hero->tempOwner, sTileObj->tempOwner) != PlayerRelations::ENEMIES))
	{
		/// TODO: Find way to reuse CPlayerSpecificInfoCallback::getTownsInfo
		/// This may be handy if we allow to use teleportation to friendly towns
		auto towns = gs->getPlayer(hero->tempOwner)->towns;
		for(const auto & town : towns)
		{
			if(town->id != sTileObj->id && town->visitingHero == nullptr
				&& town->hasBuilt(BuildingID::CASTLE_GATE, ETownType::INFERNO))
			{
				neighbours.push_back(town->visitablePos());
			}
		}
	}
}

bool CPathfinder::isLayerAvailable(const ELayer &layer, const int &turn) const
{
	switch(layer)
	{
		case ELayer::AIR:
			if(!hlp->ti->bonusFlying)
				return false;

			break;

		case ELayer::WATER:
			if(!hlp->ti->bonusWaterWalking)
				return false;
			break;
	}

	return true;
}

bool CPathfinder::isLayerTransitionPossible() const
{
	/// No layer transition allowed when previous node action is BATTLE
	if(cp->action == CGPathNode::BATTLE)
		return false;

	if((cp->layer == ELayer::AIR || cp->layer ==  ELayer::WATER)
	   && dp->layer != ELayer::LAND)
	{
		/// Hero that fly or walking on water can only go into ground layer
		return false;
	}
	else if(cp->layer == ELayer::AIR && dp->layer == ELayer::LAND)
	{
		if(options.originalMovementRules)
		{
			if ((cp->accessible != CGPathNode::ACCESSIBLE &&
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
	}
	else if(cp->layer == ELayer::LAND && dp->layer == ELayer::AIR)
	{
		if(options.lightweightFlyingMode && !isSourceInitialPosition())
			return false;
	}
	else if(cp->layer == ELayer::SAIL && dp->layer != ELayer::LAND)
		return false;
	else if(cp->layer == ELayer::SAIL && dp->layer == ELayer::LAND)
	{
		if(!dt->isCoastal())
			return false;

		//tile must be accessible -> exception: unblocked blockvis tiles -> clear but guarded by nearby monster coast
		if((dp->accessible != CGPathNode::ACCESSIBLE && (dp->accessible != CGPathNode::BLOCKVIS || dt->blocked))
			|| dt->visitable)  //TODO: passableness problem -> town says it's passable (thus accessible) but we obviously can't disembark onto town gate
			return false;
	}
	else if(cp->layer == ELayer::LAND && dp->layer == ELayer::SAIL)
	{
		if(dp->accessible == CGPathNode::ACCESSIBLE) //cannot enter empty water tile from land -> it has to be visitable
			return false;
	}
	return true;
}

bool CPathfinder::isMovementToDestPossible()
{
	auto obj = dt->topVisitableObj();
	switch(dp->layer)
	{
		case ELayer::LAND:
			if(!canMoveBetween(cp->coord, dp->coord) || dp->accessible == CGPathNode::BLOCKED)
				return false;
			if(isSourceGuarded())
			{
				if(!(options.originalMovementRules && cp->layer == ELayer::AIR) &&
					!isDestinationGuardian()) // Can step into tile of guard
				{
					return false;
				}
			}
			if(cp->layer == ELayer::SAIL)
				destAction = CGPathNode::DISEMBARK;

			break;
		case ELayer::SAIL:
			if(!canMoveBetween(cp->coord, dp->coord) || dp->accessible == CGPathNode::BLOCKED)
				return false;
			if(isSourceGuarded() && !isDestinationGuardian()) // Can step into tile of guard
				return false;

			if(cp->layer == ELayer::LAND)
			{
				if(!obj)
					return false;

				if(obj->ID == Obj::BOAT)
					destAction = CGPathNode::EMBARK;
				else if(obj->ID != Obj::HERO)
					return false;
			}
			break;

		case ELayer::AIR:
			//if(!canMoveBetween(cp->coord, dp->coord))
			//	return false;

			break;

		case ELayer::WATER:
			if(!canMoveBetween(cp->coord, dp->coord) || dp->accessible != CGPathNode::ACCESSIBLE)
				return false;
			if(isDestinationGuarded())
				return false;

			break;
	}

	if(destAction == CGPathNode::UNKNOWN)
	{
		destAction = CGPathNode::NORMAL;
		if(dp->layer == ELayer::LAND || dp->layer == ELayer::SAIL)
		{
			if(obj)
			{
				auto objRel = getPlayerRelations(obj->tempOwner, hero->tempOwner);
				if(obj->ID == Obj::HERO)
				{
					if(objRel == PlayerRelations::ENEMIES)
						destAction = CGPathNode::BATTLE;
					else
						destAction = CGPathNode::BLOCKING_VISIT;
				}
				else if(obj->ID == Obj::TOWN && objRel == PlayerRelations::ENEMIES)
				{
					const CGTownInstance * townObj = dynamic_cast<const CGTownInstance *>(obj);
					if (townObj->armedGarrison())
						destAction = CGPathNode::BATTLE;
				}
				else if(obj->ID == Obj::GARRISON || obj->ID == Obj::GARRISON2)
				{
					const CGGarrison * garrisonObj = dynamic_cast<const CGGarrison *>(obj);
					if((garrisonObj->stacksCount() && objRel == PlayerRelations::ENEMIES) || isDestinationGuarded(true))
						destAction = CGPathNode::BATTLE;
				}
				else if(isDestinationGuardian())
					destAction = CGPathNode::BATTLE;
				else if(obj->blockVisit && (!options.useCastleGate || obj->ID != Obj::TOWN))
					destAction = CGPathNode::BLOCKING_VISIT;


				if(destAction == CGPathNode::NORMAL)
				{
					if(options.originalMovementRules && isDestinationGuarded())
						destAction = CGPathNode::BATTLE;
					else
						destAction = CGPathNode::VISIT;
				}
			}
			else if(isDestinationGuarded())
				destAction = CGPathNode::BATTLE;
		}
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
		/// For now we only add visitable tile into queue when it's teleporter that allow transit
		/// Movement from visitable tile when hero is standing on it is possible into any layer
		if(CGTeleport::isTeleport(dt->topVisitableObj()))
		{
			/// For now we'll always allow transit over teleporters
			/// Transit over whirlpools only allowed when hero protected
			auto whirlpool = dynamic_cast<const CGWhirlpool *>(dt->topVisitableObj());
			if(!whirlpool || options.useTeleportWhirlpool)
				return true;
		}
		else
			return false;
	case CGPathNode::NORMAL:
		return true;

	case CGPathNode::EMBARK:
		if(options.useEmbarkAndDisembark)
			return true;

	case CGPathNode::DISEMBARK:
		if(options.useEmbarkAndDisembark && !isDestinationGuarded())
			return true;

	case CGPathNode::BATTLE:
		/// Movement after BATTLE action only possible to guardian tile
		if(isDestinationGuarded())
			return true;
	}

	return false;
}

bool CPathfinder::isSourceInitialPosition() const
{
	return cp->coord == out.hpos;
}

int3 CPathfinder::getSourceGuardPosition() const
{
	return gs->map->guardingCreaturePositions[cp->coord.x][cp->coord.y][cp->coord.z];
}

bool CPathfinder::isSourceGuarded() const
{
	//map can start with hero on guarded tile or teleport there using dimension door
	//so threat tile hero standing on like it's not guarded because it's should be possible to move out of here
	if(getSourceGuardPosition() != int3(-1, -1, -1) && !isSourceInitialPosition())
	{
		//special case -> hero embarked a boat standing on a guarded tile -> we must allow to move away from that tile
		if(cp->accessible != CGPathNode::VISITABLE
		   || cp->theNodeBefore->layer == ELayer::LAND
		   || ct->topVisitableId() != Obj::BOAT)
		{
			return true;
		}
	}

	return false;
}

bool CPathfinder::isDestinationGuarded(const bool ignoreAccessibility) const
{
	if(gs->map->guardingCreaturePositions[dp->coord.x][dp->coord.y][dp->coord.z].valid()
		&& (ignoreAccessibility || dp->accessible == CGPathNode::BLOCKVIS))
	{
		return true;
	}

	return false;
}

bool CPathfinder::isDestinationGuardian() const
{
	return getSourceGuardPosition() == dp->coord;
}

void CPathfinder::initializeGraph()
{
	auto updateNode = [&](int3 pos, ELayer layer, const TerrainTile *tinfo, bool blockNotAccessible)
	{
		auto node = out.getNode(pos, layer);
		node->reset();

		auto accessibility = evaluateAccessibility(pos, tinfo);
		/// TODO: Probably this shouldn't be handled by initializeGraph
		if(blockNotAccessible
			&& (accessibility != CGPathNode::ACCESSIBLE || tinfo->terType == ETerrainType::WATER))
			accessibility = CGPathNode::BLOCKED;
		node->accessible = accessibility;
	};

	int3 pos;
	for(pos.x=0; pos.x < out.sizes.x; ++pos.x)
	{
		for(pos.y=0; pos.y < out.sizes.y; ++pos.y)
		{
			for(pos.z=0; pos.z < out.sizes.z; ++pos.z)
			{
				const TerrainTile *tinfo = &gs->map->getTile(pos);
				switch(tinfo->terType)
				{
					case ETerrainType::ROCK:
						break;
					case ETerrainType::WATER:
						updateNode(pos, ELayer::SAIL, tinfo, false);
						if(options.useFlying)
							updateNode(pos, ELayer::AIR, tinfo, true);
						if(options.useWaterWalking)
							updateNode(pos, ELayer::WATER, tinfo, false);
						break;
					default:
						updateNode(pos, ELayer::LAND, tinfo, false);
						if(options.useFlying)
							updateNode(pos, ELayer::AIR, tinfo, true);
						break;
				}
			}
		}
	}
}

CGPathNode::EAccessibility CPathfinder::evaluateAccessibility(const int3 &pos, const TerrainTile *tinfo) const
{
	CGPathNode::EAccessibility ret = (tinfo->blocked ? CGPathNode::BLOCKED : CGPathNode::ACCESSIBLE);

	if(tinfo->terType == ETerrainType::ROCK || !isVisible(pos, hero->tempOwner))
		return CGPathNode::BLOCKED;

	if(tinfo->visitable)
	{
		if(tinfo->visitableObjects.front()->ID == Obj::SANCTUARY && tinfo->visitableObjects.back()->ID == Obj::HERO && tinfo->visitableObjects.back()->tempOwner != hero->tempOwner) //non-owned hero stands on Sanctuary
		{
			return CGPathNode::BLOCKED;
		}
		else
		{
			for(const CGObjectInstance *obj : tinfo->visitableObjects)
			{
				if(obj->passableFor(hero->tempOwner))
				{
					ret = CGPathNode::ACCESSIBLE;
				}
				else if(obj->blockVisit)
				{
					return CGPathNode::BLOCKVIS;
				}
				else if(obj->ID != Obj::EVENT) //pathfinder should ignore placed events
				{
					ret =  CGPathNode::VISITABLE;
				}
			}
		}
	}
	else if(gs->map->guardingCreaturePositions[pos.x][pos.y][pos.z].valid()
		&& !tinfo->blocked)
	{
		// Monster close by; blocked visit for battle.
		return CGPathNode::BLOCKVIS;
	}

	return ret;
}

bool CPathfinder::canMoveBetween(const int3 &a, const int3 &b) const
{
	return gs->checkForVisitableDir(a, b);
}

bool CPathfinder::addTeleportTwoWay(const CGTeleport * obj) const
{
	return options.useTeleportTwoWay && gs->isTeleportChannelBidirectional(obj->channel, hero->tempOwner);
}

bool CPathfinder::addTeleportOneWay(const CGTeleport * obj) const
{
	if(options.useTeleportOneWay && isTeleportChannelUnidirectional(obj->channel, hero->tempOwner))
	{
		auto passableExits = CGTeleport::getPassableExits(gs, hero, gs->getTeleportChannelExits(obj->channel, hero->tempOwner));
		if(passableExits.size() == 1)
			return true;
	}
	return false;
}

bool CPathfinder::addTeleportOneWayRandom(const CGTeleport * obj) const
{
	if(options.useTeleportOneWayRandom && isTeleportChannelUnidirectional(obj->channel, hero->tempOwner))
	{
		auto passableExits = CGTeleport::getPassableExits(gs, hero, gs->getTeleportChannelExits(obj->channel, hero->tempOwner));
		if(passableExits.size() > 1)
			return true;
	}
	return false;
}

bool CPathfinder::addTeleportWhirlpool(const CGWhirlpool * obj) const
{
	return options.useTeleportWhirlpool && obj;
}

bool CPathfinder::canVisitObject() const
{
	//hero can't visit objects while walking on water or flying
	return cp->layer == ELayer::LAND || cp->layer == ELayer::SAIL;
}

CPathfinderHelper::CPathfinderHelper(const CGHeroInstance * Hero)
	: ti(nullptr), hero(Hero)
{
	turnsInfo.reserve(16);
	updateTurnInfo();
}

void CPathfinderHelper::updateTurnInfo(const int &turn)
{
	if(!ti || ti->turn != turn)
	{
		if(turn < turnsInfo.size())
			ti = turnsInfo[turn];
		else
		{
			ti = getTurnInfo(hero, turn);
			turnsInfo.push_back(ti);
		}
	}
}

int CPathfinderHelper::getMaxMovePoints(const EPathfindingLayer &layer) const
{
	return layer == EPathfindingLayer::SAIL ? ti->maxMovePointsWater : ti->maxMovePointsLand;
}

TurnInfo * CPathfinderHelper::getTurnInfo(const CGHeroInstance * h, const int &turn)
{
	auto turnInfo = new TurnInfo;
	turnInfo->turn = turn;
	turnInfo->maxMovePointsLand = h->maxMovePoints(true);
	turnInfo->maxMovePointsWater = h->maxMovePoints(false);
	turnInfo->bonusFlying = h->getBonusAtTurn(Bonus::FLYING_MOVEMENT, turn);
	turnInfo->bonusWaterWalking = h->getBonusAtTurn(Bonus::WATER_WALKING, turn);
	return turnInfo;
}

void CPathfinderHelper::getNeighbours(CGameState * gs, const TerrainTile &srct, const int3 &tile, std::vector<int3> &vec, const boost::logic::tribool &onLand, const bool &limitCoastSailing)
{
	static const int3 dirs[] = { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0),
					int3(1,1,0),int3(-1,1,0),int3(1,-1,0),int3(-1,-1,0) };

	//vec.reserve(8); //optimization
	for (auto & dir : dirs)
	{
		const int3 hlp = tile + dir;
		if(!gs->isInTheMap(hlp))
			continue;

		const TerrainTile &hlpt = gs->map->getTile(hlp);

// 		//we cannot visit things from blocked tiles
// 		if(srct.blocked && !srct.visitable && hlpt.visitable && srct.blockingObjects.front()->ID != HEROI_TYPE)
// 		{
// 			continue;
// 		}

		if(srct.terType == ETerrainType::WATER && limitCoastSailing && hlpt.terType == ETerrainType::WATER && dir.x && dir.y) //diagonal move through water
		{
			int3 hlp1 = tile,
				hlp2 = tile;
			hlp1.x += dir.x;
			hlp2.y += dir.y;

			if(gs->map->getTile(hlp1).terType != ETerrainType::WATER || gs->map->getTile(hlp2).terType != ETerrainType::WATER)
				continue;
		}

		if((indeterminate(onLand)  ||  onLand == (hlpt.terType!=ETerrainType::WATER) )
			&& hlpt.terType != ETerrainType::ROCK)
		{
			vec.push_back(hlp);
		}
	}
}

int CPathfinderHelper::getMovementCost(const CGHeroInstance * h, const int3 &src, const int3 &dst, const int &remainingMovePoints, const TurnInfo * ti, const bool &checkLast)
{
	if(src == dst) //same tile
		return 0;

	if(!ti)
		ti = getTurnInfo(h);

	auto s = h->cb->getTile(src), d = h->cb->getTile(dst);
	int ret = h->getTileCost(*d, *s, ti);

	if(d->blocked && ti->bonusFlying)
	{
		ret *= (100.0 + ti->bonusFlying->val) / 100.0;
	}
	else if(d->terType == ETerrainType::WATER)
	{
		if(h->boat && s->hasFavourableWinds() && d->hasFavourableWinds()) //Favourable Winds
			ret *= 0.666;
		else if(!h->boat && ti->bonusWaterWalking)
		{
			ret *= (100.0 + ti->bonusWaterWalking->val) / 100.0;
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

	int left = remainingMovePoints-ret;
	if(checkLast && left > 0 && remainingMovePoints-ret < 250) //it might be the last tile - if no further move possible we take all move points
	{
		std::vector<int3> vec;
		vec.reserve(8); //optimization
		getNeighbours(h->cb->gameState(), *d, dst, vec, s->terType != ETerrainType::WATER, true);
		for(auto & elem : vec)
		{
			int fcost = getMovementCost(h, dst, elem, left, ti, false);
			if(fcost <= left)
				return ret;
		}
		ret = remainingMovePoints;
	}
	return ret;
}

int CPathfinderHelper::getMovementCost(const CGHeroInstance * h, const int3 &dst)
{
	return getMovementCost(h, h->visitablePos(), dst, h->movement);
}

CGPathNode::CGPathNode(int3 Coord, ELayer Layer)
	: coord(Coord), layer(Layer)
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

CPathsInfo::CPathsInfo(const int3 &Sizes)
	: sizes(Sizes)
{
	hero = nullptr;
	nodes.resize(boost::extents[sizes.x][sizes.y][sizes.z][ELayer::NUM_LAYERS]);
	for(int i = 0; i < sizes.x; i++)
		for(int j = 0; j < sizes.y; j++)
			for(int z = 0; z < sizes.z; z++)
				for(int l = 0; l < ELayer::NUM_LAYERS; l++)
					nodes[i][j][z][l] = new CGPathNode(int3(i, j, z), static_cast<ELayer>(l));
}

CPathsInfo::~CPathsInfo()
{
}

const CGPathNode * CPathsInfo::getPathInfo(const int3 &tile, const ELayer &layer) const
{
	boost::unique_lock<boost::mutex> pathLock(pathMx);

	if(tile.x >= sizes.x || tile.y >= sizes.y || tile.z >= sizes.z || layer >= ELayer::NUM_LAYERS)
		return nullptr;
	return getNode(tile, layer);
}

bool CPathsInfo::getPath(CGPath &out, const int3 &dst, const ELayer &layer) const
{
	boost::unique_lock<boost::mutex> pathLock(pathMx);

	out.nodes.clear();
	const CGPathNode *curnode = getNode(dst, layer);
	if(!curnode->theNodeBefore)
		return false;

	while(curnode)
	{
		CGPathNode cpn = *curnode;
		curnode = curnode->theNodeBefore;
		out.nodes.push_back(cpn);
	}
	return true;
}

int CPathsInfo::getDistance(const int3 &tile, const ELayer &layer) const
{
	boost::unique_lock<boost::mutex> pathLock(pathMx);

	CGPath ret;
	if(getPath(ret, tile, layer))
		return ret.nodes.size();
	else
		return 255;
}

CGPathNode *CPathsInfo::getNode(const int3 &coord, const ELayer &layer) const
{
	if(layer != ELayer::AUTO)
		return nodes[coord.x][coord.y][coord.z][layer];

	auto landNode = nodes[coord.x][coord.y][coord.z][ELayer::LAND];
	if(landNode->theNodeBefore)
		return landNode;
	else
		return nodes[coord.x][coord.y][coord.z][ELayer::SAIL];
}
