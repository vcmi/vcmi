#include "StdInc.h"
#include "AIUtility.h"
#include "VCAI.h"

#include "../../lib/UnlockGuard.h"
#include "../../lib/CObjectHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CHeroHandler.h"

/*
 * AIUtility.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper *fh;

//extern static const int3 dirs[8];

void foreach_tile_pos(std::function<void(const int3& pos)> foo)
{
	for(int i = 0; i < cb->getMapSize().x; i++)
		for(int j = 0; j < cb->getMapSize().y; j++)
			for(int k = 0; k < cb->getMapSize().z; k++)
				foo(int3(i,j,k));
}

void foreach_neighbour(const int3 &pos, std::function<void(const int3& pos)> foo)
{
	for(const int3 &dir : dirs)
	{
		const int3 n = pos + dir;
		if(cb->isInTheMap(n))
			foo(pos+dir);
	}
}

std::string strFromInt3(int3 pos)
{
	std::ostringstream oss;
	oss << pos;
	return oss.str();
}

bool isCloser(const CGObjectInstance *lhs, const CGObjectInstance *rhs)
{
	const CGPathNode *ln = cb->getPathInfo(lhs->visitablePos()), *rn = cb->getPathInfo(rhs->visitablePos());
	if(ln->turns != rn->turns)
		return ln->turns < rn->turns;

	return (ln->moveRemains > rn->moveRemains);
}

bool compareMovement(HeroPtr lhs, HeroPtr rhs)
{
	return lhs->movement > rhs->movement;
}

ui64 evaluateDanger(crint3 tile)
{
	const TerrainTile *t = cb->getTile(tile, false);
	if(!t) //we can know about guard but can't check its tile (the edge of fow)
		return 190000000; //MUCH

	ui64 objectDanger = 0, guardDanger = 0;

	auto visObjs = cb->getVisitableObjs(tile);
	if(visObjs.size())
		objectDanger = evaluateDanger(visObjs.back());

	int3 guardPos = cb->guardingCreaturePosition(tile);
	if(guardPos.x >= 0 && guardPos != tile)
		guardDanger = evaluateDanger(guardPos);

	//TODO mozna odwiedzic blockvis nie ruszajac straznika
	return std::max(objectDanger, guardDanger);

	return 0;
}

ui64 evaluateDanger(crint3 tile, const CGHeroInstance *visitor)
{
	const TerrainTile *t = cb->getTile(tile, false);
	if(!t) //we can know about guard but can't check its tile (the edge of fow)
		return 190000000; //MUCH

	ui64 objectDanger = 0, guardDanger = 0;

	auto visitableObjects = cb->getVisitableObjs(tile);
	// in some scenarios hero happens to be "under" the object (eg town). Then we consider ONLY the hero.
	if(vstd::contains_if(visitableObjects, objWithID<Obj::HERO>))
		vstd::erase_if(visitableObjects, [](const CGObjectInstance * obj)
		{
			return !objWithID<Obj::HERO>(obj);
		});

	if(const CGObjectInstance * dangerousObject = vstd::backOrNull(visitableObjects))
	{
		objectDanger = evaluateDanger(dangerousObject); //unguarded objects can also be dangerous or unhandled
		if (objectDanger)
		{
			//TODO: don't downcast objects AI shouldnt know about!
			auto armedObj = dynamic_cast<const CArmedInstance*>(dangerousObject);
			if(armedObj)
				objectDanger *= fh->getTacticalAdvantage(visitor, armedObj); //this line tends to go infinite for allied towns (?)
		}
	}

	auto guards = cb->getGuardingCreatures(tile);
	for (auto cre : guards)
	{
		vstd::amax (guardDanger, evaluateDanger(cre) * fh->getTacticalAdvantage(visitor, dynamic_cast<const CArmedInstance*>(cre))); //we are interested in strongest monster around
	}

	//TODO mozna odwiedzic blockvis nie ruszajac straznika
	return std::max(objectDanger, guardDanger);
}

ui64 evaluateDanger(const CGObjectInstance *obj)
{
	if(obj->tempOwner < PlayerColor::PLAYER_LIMIT && cb->getPlayerRelations(obj->tempOwner, ai->playerID) != PlayerRelations::ENEMIES) //owned or allied objects don't pose any threat
		return 0;

	switch(obj->ID)
	{
	case Obj::HERO:
		{
			InfoAboutHero iah;
			cb->getHeroInfo(obj, iah);
			return iah.army.getStrength();
		}
	case Obj::TOWN:
	case Obj::GARRISON: case Obj::GARRISON2: //garrison
		{
			InfoAboutTown iat;
			cb->getTownInfo(obj, iat);
			return iat.army.getStrength();
		}
	case Obj::MONSTER:
		{
			//TODO!!!!!!!!
			const CGCreature *cre = dynamic_cast<const CGCreature*>(obj);
			return cre->getArmyStrength();
		}
	case Obj::CREATURE_GENERATOR1:
		{
			const CGDwelling *d = dynamic_cast<const CGDwelling*>(obj);
			return d->getArmyStrength();
		}
	case Obj::MINE:
	case Obj::ABANDONED_MINE:
		{
			const CArmedInstance * a = dynamic_cast<const CArmedInstance*>(obj);
			return a->getArmyStrength();
		}
	case Obj::CRYPT: //crypt
	case Obj::CREATURE_BANK: //crebank
	case Obj::DRAGON_UTOPIA:
	case Obj::SHIPWRECK: //shipwreck
	case Obj::DERELICT_SHIP: //derelict ship
//	case Obj::PYRAMID:
		return fh->estimateBankDanger (VLC->objh->bankObjToIndex(obj));
	case Obj::PYRAMID:
		{
		    if(obj->subID == 0)
				return fh->estimateBankDanger (VLC->objh->bankObjToIndex(obj));
			else
				return 0;
		}
	default:
		return 0;
	}
}

bool compareDanger(const CGObjectInstance *lhs, const CGObjectInstance *rhs)
{
	return evaluateDanger(lhs) < evaluateDanger(rhs);
}

bool isSafeToVisit(HeroPtr h, crint3 tile)
{
	const ui64 heroStrength = h->getTotalStrength(),
		dangerStrength = evaluateDanger(tile, *h);
	if(dangerStrength)
	{
		if(heroStrength / SAFE_ATTACK_CONSTANT > dangerStrength)
		{
            logAi->debugStream() << boost::format("It's, safe for %s to visit tile %s") % h->name % tile;
			return true;
		}
		else
			return false;
	}

	return true; //there's no danger
}

bool isReachable(const CGObjectInstance *obj)
{
	return cb->getPathInfo(obj->visitablePos())->turns < 255;
}

bool canBeEmbarkmentPoint(const TerrainTile *t)
{
	//tile must be free of with unoccupied boat
	return !t->blocked
        || (t->visitableObjects.size() == 1 && t->topVisitableId() == Obj::BOAT);
}

int3 whereToExplore(HeroPtr h)
{
	//TODO it's stupid and ineffective, write sth better
	cb->setSelection(*h);
	int radius = h->getSightRadious();
	int3 hpos = h->visitablePos();

	//look for nearby objs -> visit them if they're close enouh
	const int DIST_LIMIT = 3;
	std::vector<const CGObjectInstance *> nearbyVisitableObjs;
	for(const CGObjectInstance *obj : ai->getPossibleDestinations(h))
	{
		int3 op = obj->visitablePos();
		CGPath p;
		cb->getPath2(op, p);
		if(p.nodes.size() && p.endPos() == op && p.nodes.size() <= DIST_LIMIT)
			nearbyVisitableObjs.push_back(obj);
	}
	boost::sort(nearbyVisitableObjs, isCloser);
	if(nearbyVisitableObjs.size())
		return nearbyVisitableObjs.back()->visitablePos();

	try
	{
		return ai->explorationBestNeighbour(hpos, radius, h);
	}
	catch(cannotFulfillGoalException &e)
	{
		std::vector<std::vector<int3> > tiles; //tiles[distance_to_fow]
		try
		{
			return ai->explorationNewPoint(radius, h, tiles);
		}
		catch(cannotFulfillGoalException &e)
		{
			std::map<int, std::vector<int3> > profits;
			{
				TimeCheck tc("Evaluating exploration possibilities");
				tiles[0].clear(); //we can't reach FoW anyway
				for(auto &vt : tiles)
					for(auto &tile : vt)
						profits[howManyTilesWillBeDiscovered(tile, radius)].push_back(tile);
			}

			if(profits.empty())
				return int3 (-1,-1,-1);

			auto bestDest = profits.end();
			bestDest--;
			return bestDest->second.front(); //TODO which is the real best tile?
		}
	}
}

bool isBlockedBorderGate(int3 tileToHit)
{
    return cb->getTile(tileToHit)->topVisitableId() == Obj::BORDER_GATE
		&& cb->getPathInfo(tileToHit)->accessible != CGPathNode::ACCESSIBLE;
}

int howManyTilesWillBeDiscovered(const int3 &pos, int radious)
{ //TODO: do not explore dead-end boundaries
	int ret = 0;
	for(int x = pos.x - radious; x <= pos.x + radious; x++)
	{
		for(int y = pos.y - radious; y <= pos.y + radious; y++)
		{
			int3 npos = int3(x,y,pos.z);
			if(cb->isInTheMap(npos) && pos.dist2d(npos) - 0.5 < radious  && !cb->isVisible(npos))
			{
				if (!boundaryBetweenTwoPoints (pos, npos))
					ret++;
			}
		}
	}

	return ret;
}

bool boundaryBetweenTwoPoints (int3 pos1, int3 pos2) //determines if two points are separated by known barrier
{
	int xMin = std::min (pos1.x, pos2.x);
	int xMax = std::max (pos1.x, pos2.x);
	int yMin = std::min (pos1.y, pos2.y);
	int yMax = std::max (pos1.y, pos2.y);

	for (int x = xMin; x <= xMax; ++x)
	{
		for (int y = yMin; y <= yMax; ++y)
		{
			int3 tile = int3(x, y, pos1.z); //use only on same level, ofc
			if (abs(pos1.dist2d(tile) - pos2.dist2d(tile)) < 1.5)
			{
				if (!(cb->isVisible(tile) && cb->getTile(tile)->blocked)) //if there's invisible or unblocked tile inbetween, it's good 
					return false;
			}
		}
	}
	return true; //if all are visible and blocked, we're at dead end
}

int howManyTilesWillBeDiscovered(int radious, int3 pos, crint3 dir)
{
	return howManyTilesWillBeDiscovered(pos + dir, radious);
}

void getVisibleNeighbours(const std::vector<int3> &tiles, std::vector<int3> &out)
{
	for(const int3 &tile : tiles)
	{
		foreach_neighbour(tile, [&](int3 neighbour)
		{
			if(cb->isVisible(neighbour))
				out.push_back(neighbour);
		});
	}
}

ui64 howManyReinforcementsCanGet(HeroPtr h, const CGTownInstance *t)
{
	ui64 ret = 0;
	int freeHeroSlots = GameConstants::ARMY_SIZE - h->stacksCount();
	std::vector<const CStackInstance *> toMove;
	for(auto const slot : t->Slots())
	{
		//can be merged woth another stack?
		SlotID dst = h->getSlotFor(slot.second->getCreatureID());
		if(h->hasStackAtSlot(dst))
			ret += t->getPower(slot.first);
		else
			toMove.push_back(slot.second);
	}
	boost::sort(toMove, [](const CStackInstance *lhs, const CStackInstance *rhs)
	{
		return lhs->getPower() < rhs->getPower();
	});
	for (auto & stack : boost::adaptors::reverse(toMove))
	{
		if(freeHeroSlots)
		{
			ret += stack->getPower();
			freeHeroSlots--;
		}
		else
			break;
	}
	return ret;
}

bool compareHeroStrength(HeroPtr h1, HeroPtr h2)
{
	return h1->getTotalStrength() < h2->getTotalStrength();
}

bool compareArmyStrength(const CArmedInstance *a1, const CArmedInstance *a2)
{
	return a1->getArmyStrength() < a2->getArmyStrength();
}
