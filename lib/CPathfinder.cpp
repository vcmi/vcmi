#include "StdInc.h"
#include "CPathfinder.h"

#include "CHeroHandler.h"
#include "mapping/CMap.h"
#include "registerTypes/RegisterTypes.h"
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
}

CPathfinder::CPathfinder(CPathsInfo &_out, CGameState *_gs, const CGHeroInstance *_hero) : CGameInfoCallback(_gs, boost::optional<PlayerColor>()), out(_out), hero(_hero), FoW(getPlayerTeam(hero->tempOwner)->fogOfWarMap)
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

	initializeGraph();

	if(hero->canFly())
		options.useFlying = true;
	if(hero->canWalkOnSea())
		options.useWaterWalking = true;
	if(CGWhirlpool::isProtected(hero))
		options.useTeleportWhirlpool = true;

	neighbours.reserve(16);
}

void CPathfinder::calculatePaths()
{
	int maxMovePointsLand = hero->maxMovePoints(true);
	int maxMovePointsWater = hero->maxMovePoints(false);

	auto maxMovePoints = [&](CGPathNode *cp) -> int
	{
		return cp->land ? maxMovePointsLand : maxMovePointsWater;
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
	CGPathNode &initialNode = *getNode(out.hpos);
	initialNode.turns = 0;
	initialNode.moveRemains = hero->movement;
	mq.push_back(&initialNode);

	while(!mq.empty())
	{
		cp = mq.front();
		mq.pop_front();

		int movement = cp->moveRemains, turn = cp->turns;
		if(!movement)
		{
			movement = maxMovePoints(cp);
			turn++;
		}

		//add accessible neighbouring nodes to the queue
		addNeighbours(cp->coord);
		for(auto & neighbour : neighbours)
		{
			dp = getNode(neighbour);
			dt = &gs->map->getTile(neighbour);
			useEmbarkCost = 0; //0 - usual movement; 1 - embark; 2 - disembark

			if(!isMovementPossible())
				continue;

			int cost = gs->getMovementCost(hero, cp->coord, dp->coord, movement);
			int remains = movement - cost;
			if(useEmbarkCost)
			{
				remains = hero->movementPointsAfterEmbark(movement, cost, useEmbarkCost - 1);
				cost = movement - remains;
			}

			int turnAtNextTile = turn;
			if(remains < 0)
			{
				//occurs rarely, when hero with low movepoints tries to leave the road
				turnAtNextTile++;
				int moveAtNextTile = maxMovePoints(cp);
				cost = gs->getMovementCost(hero, cp->coord, dp->coord, moveAtNextTile); //cost must be updated, movement points changed :(
				remains = moveAtNextTile - cost;
			}

			if(isBetterWay(remains, turnAtNextTile))
			{
				assert(dp != cp->theNodeBefore); //two tiles can't point to each other
				dp->moveRemains = remains;
				dp->turns = turnAtNextTile;
				dp->theNodeBefore = cp;

				if(checkDestinationTile())
					mq.push_back(dp);
			}
		} //neighbours loop

		//just add all passable teleport exits
		if(sTileObj)
		{
			addTeleportExits();
			for(auto & neighbour : neighbours)
			{
				dp = getNode(neighbour);
				if(isBetterWay(movement, turn))
				{
					dp->moveRemains = movement;
					dp->turns = turn;
					dp->theNodeBefore = cp;
					mq.push_back(dp);
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
	gs->getNeighbours(*ct, coord, tiles, boost::logic::indeterminate, !cp->land);
	sTileObj = ct->topVisitableObj(coord == CGHeroInstance::convertPosition(hero->pos, false));
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
}

bool CPathfinder::isMovementPossible()
{
	if(!canMoveBetween(cp->coord, dp->coord) || dp->accessible == CGPathNode::BLOCKED)
		return false;

	Obj destTopVisObjID = dt->topVisitableId();
	if(cp->land != dp->land) //hero can traverse land<->sea only in special circumstances
	{
		if(cp->land) //from land to sea -> embark or assault hero on boat
		{
			if(dp->accessible == CGPathNode::ACCESSIBLE || destTopVisObjID < 0) //cannot enter empty water tile from land -> it has to be visitable
				return false;
			if(destTopVisObjID != Obj::HERO && destTopVisObjID != Obj::BOAT) //only boat or hero can be accessed from land
				return false;
			if(destTopVisObjID == Obj::BOAT)
				useEmbarkCost = 1;
		}
		else //disembark
		{
			//can disembark only on coastal tiles
			if(!dt->isCoastal())
				return false;

			//tile must be accessible -> exception: unblocked blockvis tiles -> clear but guarded by nearby monster coast
			if((dp->accessible != CGPathNode::ACCESSIBLE && (dp->accessible != CGPathNode::BLOCKVIS || dt->blocked))
				|| dt->visitable)  //TODO: passableness problem -> town says it's passable (thus accessible) but we obviously can't disembark onto town gate
				return false;;

			useEmbarkCost = 2;
		}
	}

	if(isSourceGuarded() && !isDestinationGuardian()) // Can step into tile of guard
		return false;

	return true;
}

bool CPathfinder::checkDestinationTile()
{
	if(dp->accessible == CGPathNode::ACCESSIBLE)
		return true;
	if(dp->coord == CGHeroInstance::convertPosition(hero->pos, false))
		return true; // This one is tricky, we can ignore fact that tile is not ACCESSIBLE in case if it's our hero block it. Though this need investigation
	if(dp->accessible == CGPathNode::VISITABLE && CGTeleport::isTeleport(dt->topVisitableObj()))
		return true; // For now we'll always allow transit for teleporters
	if(useEmbarkCost && options.useEmbarkAndDisembark)
		return true;
	if(isDestinationGuarded() && !isSourceGuarded())
		return true; // Can step into a hostile tile once

	return false;
}

int3 CPathfinder::getSourceGuardPosition()
{
	return gs->map->guardingCreaturePositions[cp->coord.x][cp->coord.y][cp->coord.z];
}

bool CPathfinder::isSourceGuarded()
{
	//map can start with hero on guarded tile or teleport there using dimension door
	//so threat tile hero standing on like it's not guarded because it's should be possible to move out of here
	if(getSourceGuardPosition() != int3(-1, -1, -1)
		&& cp->coord != hero->getPosition(false))
	{
		//special case -> hero embarked a boat standing on a guarded tile -> we must allow to move away from that tile
		if(cp->accessible != CGPathNode::VISITABLE
		   || !cp->theNodeBefore->land
		   || ct->topVisitableId() != Obj::BOAT)
		{
			return true;
		}
	}

	return false;
}

bool CPathfinder::isDestinationGuarded()
{
	if(gs->map->guardingCreaturePositions[dp->coord.x][dp->coord.y][dp->coord.z].valid()
		&& dp->accessible == CGPathNode::BLOCKVIS)
	{
		return true;
	}

	return false;
}

bool CPathfinder::isDestinationGuardian()
{
	return getSourceGuardPosition() == dp->coord;
}

void CPathfinder::initializeGraph()
{
	int3 pos;
	CGPathNode ***graph = out.nodes;
	for(pos.x=0; pos.x < out.sizes.x; ++pos.x)
	{
		for(pos.y=0; pos.y < out.sizes.y; ++pos.y)
		{
			for(pos.z=0; pos.z < out.sizes.z; ++pos.z)
			{
				const TerrainTile *tinfo = &gs->map->getTile(pos);
				CGPathNode &node = graph[pos.x][pos.y][pos.z];
				node.accessible = evaluateAccessibility(pos, tinfo);
				node.turns = 0xff;
				node.moveRemains = 0;
				node.coord = pos;
				node.land = tinfo->terType != ETerrainType::WATER;
				node.theNodeBefore = nullptr;
			}
		}
	}
}

CGPathNode *CPathfinder::getNode(const int3 &coord)
{
	return &out.nodes[coord.x][coord.y][coord.z];
}

CGPathNode::EAccessibility CPathfinder::evaluateAccessibility(const int3 &pos, const TerrainTile *tinfo) const
{
	CGPathNode::EAccessibility ret = (tinfo->blocked ? CGPathNode::BLOCKED : CGPathNode::ACCESSIBLE);


	if(tinfo->terType == ETerrainType::ROCK || !FoW[pos.x][pos.y][pos.z])
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

CGPathNode::CGPathNode()
:coord(-1,-1,-1)
{
	accessible = NOT_SET;
	land = 0;
	moveRemains = 0;
	turns = 255;
	theNodeBefore = nullptr;
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

void CGPath::convert( ui8 mode )
{
	if(mode==0)
	{
		for(auto & elem : nodes)
		{
			elem.coord = CGHeroInstance::convertPosition(elem.coord,true);
		}
	}
}

CPathsInfo::CPathsInfo( const int3 &Sizes )
:sizes(Sizes)
{
	hero = nullptr;
	nodes = new CGPathNode**[sizes.x];
	for(int i = 0; i < sizes.x; i++)
	{
		nodes[i] = new CGPathNode*[sizes.y];
		for(int j = 0; j < sizes.y; j++)
		{
			nodes[i][j] = new CGPathNode[sizes.z];
		}
	}
}

CPathsInfo::~CPathsInfo()
{
	for(int i = 0; i < sizes.x; i++)
	{
		for(int j = 0; j < sizes.y; j++)
		{
			delete [] nodes[i][j];
		}
		delete [] nodes[i];
	}
	delete [] nodes;
}

const CGPathNode * CPathsInfo::getPathInfo( int3 tile ) const
{
	boost::unique_lock<boost::mutex> pathLock(pathMx);

	if(tile.x >= sizes.x || tile.y >= sizes.y || tile.z >= sizes.z)
		return nullptr;
	return &nodes[tile.x][tile.y][tile.z];
}

bool CPathsInfo::getPath( const int3 &dst, CGPath &out ) const
{
	boost::unique_lock<boost::mutex> pathLock(pathMx);

	out.nodes.clear();
	const CGPathNode *curnode = &nodes[dst.x][dst.y][dst.z];
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

int CPathsInfo::getDistance( int3 tile ) const
{
	boost::unique_lock<boost::mutex> pathLock(pathMx);

	CGPath ret;
	if(getPath(tile, ret))
		return ret.nodes.size();
	else
		return 255;
}
