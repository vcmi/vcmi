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

CPathfinder::CPathfinder(CPathsInfo & _out, CGameState * _gs, const CGHeroInstance * _hero)
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
	if(hlp->hasBonusOfType(Bonus::FLYING_MOVEMENT))
		options.useFlying = true;
	if(hlp->hasBonusOfType(Bonus::WATER_WALKING))
		options.useWaterWalking = true;
	if(hlp->hasBonusOfType(Bonus::WHIRLPOOL_PROTECTION))
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
		else if(dp->turns >= turn && dp->moveRemains < remains) //this route is faster
			return true;

		return false;
	};

	//logGlobal->infoStream() << boost::format("Calculating paths for hero %s (adress  %d) of player %d") % hero->name % hero % hero->tempOwner;

	//initial tile - set cost on 0 and add to the queue
	CGPathNode * initialNode = out.getNode(out.hpos, hero->boat ? ELayer::SAIL : ELayer::LAND);
	initialNode->turns = 0;
	initialNode->moveRemains = hero->movement;
	pq.push(initialNode);

	while(!pq.empty())
	{
		cp = pq.top();
		pq.pop();
		cp->locked = true;
		ct = &gs->map->getTile(cp->coord);
		cObj = ct->topVisitableObj(cp->coord == out.hpos);

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
			dObj = dt->topVisitableObj();
			for(ELayer i = ELayer::LAND; i <= ELayer::AIR; i.advance(1))
			{
				dp = out.getNode(neighbour, i);
				if(dp->accessible == CGPathNode::NOT_SET)
					continue;

				if(dp->locked)
					continue;

				if(!passOneTurnLimitCheck(cp->turns != turn))
					continue;

				if(!hlp->isLayerAvailable(i))
					continue;

				if(cp->layer != i && !isLayerTransitionPossible())
					continue;

				if(!isMovementToDestPossible())
					continue;

				destAction = getDestAction();
				int cost = CPathfinderHelper::getMovementCost(hero, cp->coord, dp->coord, movement, hlp->getTurnInfo());
				int remains = movement - cost;
				if(destAction == CGPathNode::EMBARK || destAction == CGPathNode::DISEMBARK)
				{
					remains = hero->movementPointsAfterEmbark(movement, cost, destAction - 1, hlp->getTurnInfo());
					cost = movement - remains;
				}
				int turnAtNextTile = turn;
				if(remains < 0)
				{
					//occurs rarely, when hero with low movepoints tries to leave the road
					hlp->updateTurnInfo(++turnAtNextTile);
					int moveAtNextTile = hlp->getMaxMovePoints(i);
					cost = CPathfinderHelper::getMovementCost(hero, cp->coord, dp->coord, moveAtNextTile, hlp->getTurnInfo()); //cost must be updated, movement points changed :(
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
		if(cObj && canVisitObject())
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

void CPathfinder::addNeighbours(const int3 & coord)
{
	neighbours.clear();
	std::vector<int3> tiles;
	CPathfinderHelper::getNeighbours(gs, *ct, coord, tiles, boost::logic::indeterminate, cp->layer == ELayer::SAIL); // TODO: find out if we still need "limitCoastSailing" option
	if(canVisitObject())
	{
		if(cObj)
		{
			for(int3 tile: tiles)
			{
				if(canMoveBetween(tile, cObj->visitablePos()))
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
	assert(cObj);

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

	const CGTeleport * sTileTeleport = dynamic_cast<const CGTeleport *>(cObj);
	if(isAllowedTeleportEntrance(sTileTeleport))
	{
		for(auto objId : gs->getTeleportChannelExits(sTileTeleport->channel, hero->tempOwner))
		{
			auto obj = getObj(objId);
			if(dynamic_cast<const CGWhirlpool *>(obj))
			{
				auto pos = obj->getBlockedPos();
				for(auto p : pos)
				{
					if(gs->getTile(p)->topVisitableId() == obj->ID)
						neighbours.push_back(p);
				}
			}
			else if(CGTeleport::isExitPassable(gs, hero, obj))
				neighbours.push_back(obj->visitablePos());
		}
	}

	if(options.useCastleGate
		&& (cObj->ID == Obj::TOWN && cObj->subID == ETownType::INFERNO
		&& getPlayerRelations(hero->tempOwner, cObj->tempOwner) != PlayerRelations::ENEMIES))
	{
		/// TODO: Find way to reuse CPlayerSpecificInfoCallback::getTownsInfo
		/// This may be handy if we allow to use teleportation to friendly towns
		auto towns = gs->getPlayer(hero->tempOwner)->towns;
		for(const auto & town : towns)
		{
			if(town->id != cObj->id && town->visitingHero == nullptr
				&& town->hasBuilt(BuildingID::CASTLE_GATE, ETownType::INFERNO))
			{
				neighbours.push_back(town->visitablePos());
			}
		}
	}
}

bool CPathfinder::isLayerTransitionPossible() const
{
	/// No layer transition allowed when previous node action is BATTLE
	if(cp->action == CGPathNode::BATTLE)
		return false;

	switch(cp->layer)
	{
	case ELayer::LAND:
		if(options.lightweightFlyingMode && dp->layer == ELayer::AIR)
		{
			if(!isSourceInitialPosition())
				return false;
		}
		else if(dp->layer == ELayer::SAIL)
		{
			/// Cannot enter empty water tile from land -> it has to be visitable
			if(dp->accessible == CGPathNode::ACCESSIBLE)
				return false;
		}

		break;

	case ELayer::SAIL:
		if(dp->layer != ELayer::LAND)
			return false;

		if(!dt->isCoastal())
			return false;

		//tile must be accessible -> exception: unblocked blockvis tiles -> clear but guarded by nearby monster coast
		if((dp->accessible != CGPathNode::ACCESSIBLE && (dp->accessible != CGPathNode::BLOCKVIS || dt->blocked))
			|| dt->visitable)  //TODO: passableness problem -> town says it's passable (thus accessible) but we obviously can't disembark onto town gate
		{
			return false;
		}

		break;

	case ELayer::AIR:
		if(dp->layer != ELayer::LAND)
			return false;

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
		if(dp->layer != ELayer::LAND)
			return false;

		break;
	}

	return true;
}

bool CPathfinder::isMovementToDestPossible() const
{
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

		break;

	case ELayer::SAIL:
		if(!canMoveBetween(cp->coord, dp->coord) || dp->accessible == CGPathNode::BLOCKED)
			return false;
		if(isSourceGuarded())
		{
			// Hero embarked a boat standing on a guarded tile -> we must allow to move away from that tile
			if(cp->action != CGPathNode::EMBARK && !isDestinationGuardian())
				return false;
		}

		if(cp->layer == ELayer::LAND)
		{
			if(!dObj)
				return false;

			if(dObj->ID != Obj::BOAT && dObj->ID != Obj::HERO)
				return false;
		}
		else if(dObj && dObj->ID == Obj::BOAT)
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
		/// For now we only add visitable tile into queue when it's teleporter that allow transit
		/// Movement from visitable tile when hero is standing on it is possible into any layer
		if(CGTeleport::isTeleport(dObj))
		{
			/// For now we'll always allow transit over teleporters
			/// Transit over whirlpools only allowed when hero protected
			auto whirlpool = dynamic_cast<const CGWhirlpool *>(dObj);
			if(!whirlpool || options.useTeleportWhirlpool)
				return true;
		}

		break;

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
		/// Movement after BATTLE action only possible to guardian tile
		if(isDestinationGuardian())
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

	case ELayer::SAIL:
		if(dObj)
		{
			auto objRel = getPlayerRelations(dObj->tempOwner, hero->tempOwner);

			if(dObj->ID == Obj::BOAT)
				action = CGPathNode::EMBARK;
			else if(dObj->ID == Obj::HERO)
			{
				if(objRel == PlayerRelations::ENEMIES)
					action = CGPathNode::BATTLE;
				else
					action = CGPathNode::BLOCKING_VISIT;
			}
			else if(dObj->ID == Obj::TOWN && objRel == PlayerRelations::ENEMIES)
			{
				const CGTownInstance * townObj = dynamic_cast<const CGTownInstance *>(dObj);
				if(townObj->armedGarrison())
					action = CGPathNode::BATTLE;
			}
			else if(dObj->ID == Obj::GARRISON || dObj->ID == Obj::GARRISON2)
			{
				const CGGarrison * garrisonObj = dynamic_cast<const CGGarrison *>(dObj);
				if((garrisonObj->stacksCount() && objRel == PlayerRelations::ENEMIES) || isDestinationGuarded(true))
					action = CGPathNode::BATTLE;
			}
			else if(isDestinationGuardian())
				action = CGPathNode::BATTLE;
			else if(dObj->blockVisit && (!options.useCastleGate || dObj->ID != Obj::TOWN))
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
	/// Hero can move from guarded tile if movement started on that tile
	/// It's possible at least in these cases:
	/// - Map start with hero on guarded tile
	/// - Dimention door used
	///  TODO: check what happen when there is several guards
	if(getSourceGuardPosition() != int3(-1, -1, -1) && !isSourceInitialPosition())
	{
		return true;
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
	auto updateNode = [&](int3 pos, ELayer layer, const TerrainTile * tinfo, bool blockNotAccessible)
	{
		auto node = out.getNode(pos, layer);

		auto accessibility = evaluateAccessibility(pos, tinfo);
		/// TODO: Probably this shouldn't be handled by initializeGraph
		if(blockNotAccessible
			&& (accessibility != CGPathNode::ACCESSIBLE || tinfo->terType == ETerrainType::WATER))
		{
			accessibility = CGPathNode::BLOCKED;
		}
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

CGPathNode::EAccessibility CPathfinder::evaluateAccessibility(const int3 & pos, const TerrainTile * tinfo) const
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
			for(const CGObjectInstance * obj : tinfo->visitableObjects)
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
					ret = CGPathNode::VISITABLE;
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

bool CPathfinder::canMoveBetween(const int3 & a, const int3 & b) const
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

TurnInfo::TurnInfo(const CGHeroInstance * Hero, const int turn)
	: hero(Hero), maxMovePointsLand(-1), maxMovePointsWater(-1)
{
	bonuses = hero->getAllBonuses(Selector::days(turn), nullptr);
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
	return bonuses->getFirst(Selector::type(type).And(Selector::subtype(subtype)));
}

int TurnInfo::valOfBonuses(Bonus::BonusType type, int subtype) const
{
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

CPathfinderHelper::CPathfinderHelper(const CGHeroInstance * Hero)
	: turn(-1), hero(Hero)
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

void CPathfinderHelper::getNeighbours(CGameState * gs, const TerrainTile & srct, const int3 & tile, std::vector<int3> & vec, const boost::logic::tribool & onLand, const bool limitCoastSailing)
{
	static const int3 dirs[] = { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0),
					int3(1,1,0),int3(-1,1,0),int3(1,-1,0),int3(-1,-1,0) };

	//vec.reserve(8); //optimization
	for(auto & dir : dirs)
	{
		const int3 hlp = tile + dir;
		if(!gs->isInTheMap(hlp))
			continue;

		const TerrainTile & hlpt = gs->map->getTile(hlp);

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

		if((indeterminate(onLand) || onLand == (hlpt.terType!=ETerrainType::WATER) )
			&& hlpt.terType != ETerrainType::ROCK)
		{
			vec.push_back(hlp);
		}
	}
}

int CPathfinderHelper::getMovementCost(const CGHeroInstance * h, const int3 & src, const int3 & dst, const int remainingMovePoints, const TurnInfo * ti, const bool checkLast)
{
	if(src == dst) //same tile
		return 0;

	if(!ti)
		ti = new TurnInfo(h);

	auto s = h->cb->getTile(src), d = h->cb->getTile(dst);
	int ret = h->getTileCost(*d, *s, ti);

	if(d->blocked && ti->hasBonusOfType(Bonus::FLYING_MOVEMENT))
	{
		ret *= (100.0 + ti->valOfBonuses(Bonus::FLYING_MOVEMENT)) / 100.0;
	}
	else if(d->terType == ETerrainType::WATER)
	{
		if(h->boat && s->hasFavourableWinds() && d->hasFavourableWinds()) //Favourable Winds
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

int CPathfinderHelper::getMovementCost(const CGHeroInstance * h, const int3 & dst)
{
	return getMovementCost(h, h->visitablePos(), dst, h->movement);
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
