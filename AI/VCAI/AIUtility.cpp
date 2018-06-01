/*
 * AIUtility.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AIUtility.h"
#include "VCAI.h"
#include "Fuzzy.h"

#include "../../lib/UnlockGuard.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/mapObjects/CBank.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CQuest.h"
#include "../../lib/CPathfinder.h"
#include "../../lib/mapping/CMapDefines.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

//extern static const int3 dirs[8];

const CGObjectInstance * ObjectIdRef::operator->() const
{
	return cb->getObj(id, false);
}

ObjectIdRef::operator const CGObjectInstance *() const
{
	return cb->getObj(id, false);
}

ObjectIdRef::ObjectIdRef(ObjectInstanceID _id)
	: id(_id)
{

}

ObjectIdRef::ObjectIdRef(const CGObjectInstance * obj)
	: id(obj->id)
{

}

bool ObjectIdRef::operator<(const ObjectIdRef & rhs) const
{
	return id < rhs.id;
}

HeroPtr::HeroPtr(const CGHeroInstance * H)
{
	if(!H)
	{
		//init from nullptr should equal to default init
		*this = HeroPtr();
		return;
	}

	h = H;
	name = h->name;
	hid = H->id;
//	infosCount[ai->playerID][hid]++;
}

HeroPtr::HeroPtr()
{
	h = nullptr;
	hid = ObjectInstanceID();
}

HeroPtr::~HeroPtr()
{
//	if(hid >= 0)
//		infosCount[ai->playerID][hid]--;
}

bool HeroPtr::operator<(const HeroPtr & rhs) const
{
	return hid < rhs.hid;
}

const CGHeroInstance * HeroPtr::get(bool doWeExpectNull) const
{
	//TODO? check if these all assertions every time we get info about hero affect efficiency
	//
	//behave terribly when attempting unauthorized access to hero that is not ours (or was lost)
	assert(doWeExpectNull || h);

	if(h)
	{
		auto obj = cb->getObj(hid);
		const bool owned = obj && obj->tempOwner == ai->playerID;

		if(doWeExpectNull && !owned)
		{
			return nullptr;
		}
		else
		{
			assert(obj);
			assert(owned);
		}
	}

	return h;
}

const CGHeroInstance * HeroPtr::operator->() const
{
	return get();
}

bool HeroPtr::validAndSet() const
{
	return get(true);
}

const CGHeroInstance * HeroPtr::operator*() const
{
	return get();
}

void foreach_tile_pos(std::function<void(const int3 & pos)> foo)
{
	// some micro-optimizations since this function gets called a LOT
	// callback pointer is thread-specific and slow to retrieve -> read map size only once
	int3 mapSize = cb->getMapSize();
	for(int i = 0; i < mapSize.x; i++)
	{
		for(int j = 0; j < mapSize.y; j++)
		{
			for(int k = 0; k < mapSize.z; k++)
				foo(int3(i, j, k));
		}
	}
}

void foreach_tile_pos(CCallback * cbp, std::function<void(CCallback * cbp, const int3 & pos)> foo)
{
	int3 mapSize = cbp->getMapSize();
	for(int i = 0; i < mapSize.x; i++)
	{
		for(int j = 0; j < mapSize.y; j++)
		{
			for(int k = 0; k < mapSize.z; k++)
				foo(cbp, int3(i, j, k));
		}
	}
}

void foreach_neighbour(const int3 & pos, std::function<void(const int3 & pos)> foo)
{
	CCallback * cbp = cb.get(); // avoid costly retrieval of thread-specific pointer
	for(const int3 & dir : int3::getDirs())
	{
		const int3 n = pos + dir;
		if(cbp->isInTheMap(n))
			foo(pos + dir);
	}
}

void foreach_neighbour(CCallback * cbp, const int3 & pos, std::function<void(CCallback * cbp, const int3 & pos)> foo)
{
	for(const int3 & dir : int3::getDirs())
	{
		const int3 n = pos + dir;
		if(cbp->isInTheMap(n))
			foo(cbp, pos + dir);
	}
}

bool CDistanceSorter::operator()(const CGObjectInstance * lhs, const CGObjectInstance * rhs)
{
	const CGPathNode * ln = ai->myCb->getPathsInfo(hero)->getPathInfo(lhs->visitablePos());
	const CGPathNode * rn = ai->myCb->getPathsInfo(hero)->getPathInfo(rhs->visitablePos());

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
	const TerrainTile * t = cb->getTile(tile, false);
	if(!t) //we can know about guard but can't check its tile (the edge of fow)
		return 190000000; //MUCH

	ui64 objectDanger = 0;
	ui64 guardDanger = 0;

	auto visObjs = cb->getVisitableObjs(tile);
	if(visObjs.size())
		objectDanger = evaluateDanger(visObjs.back());

	int3 guardPos = cb->getGuardingCreaturePosition(tile);
	if(guardPos.x >= 0 && guardPos != tile)
		guardDanger = evaluateDanger(guardPos);

	//TODO mozna odwiedzic blockvis nie ruszajac straznika
	return std::max(objectDanger, guardDanger);
}

ui64 evaluateDanger(crint3 tile, const CGHeroInstance * visitor)
{
	const TerrainTile * t = cb->getTile(tile, false);
	if(!t) //we can know about guard but can't check its tile (the edge of fow)
		return 190000000; //MUCH

	ui64 objectDanger = 0;
	ui64 guardDanger = 0;

	auto visitableObjects = cb->getVisitableObjs(tile);
	// in some scenarios hero happens to be "under" the object (eg town). Then we consider ONLY the hero.
	if(vstd::contains_if(visitableObjects, objWithID<Obj::HERO>))
	{
		vstd::erase_if(visitableObjects, [](const CGObjectInstance * obj)
		{
			return !objWithID<Obj::HERO>(obj);
		});
	}

	if(const CGObjectInstance * dangerousObject = vstd::backOrNull(visitableObjects))
	{
		objectDanger = evaluateDanger(dangerousObject); //unguarded objects can also be dangerous or unhandled
		if(objectDanger)
		{
			//TODO: don't downcast objects AI shouldn't know about!
			auto armedObj = dynamic_cast<const CArmedInstance *>(dangerousObject);
			if(armedObj)
			{
				float tacticalAdvantage = fh->getTacticalAdvantage(visitor, armedObj);
				objectDanger *= tacticalAdvantage; //this line tends to go infinite for allied towns (?)
			}
		}
		if(dangerousObject->ID == Obj::SUBTERRANEAN_GATE)
		{
			//check guard on the other side of the gate
			auto it = ai->knownSubterraneanGates.find(dangerousObject);
			if(it != ai->knownSubterraneanGates.end())
			{
				auto guards = cb->getGuardingCreatures(it->second->visitablePos());
				for(auto cre : guards)
				{
					vstd::amax(guardDanger, evaluateDanger(cre) * fh->getTacticalAdvantage(visitor, dynamic_cast<const CArmedInstance *>(cre)));
				}
			}
		}
	}

	auto guards = cb->getGuardingCreatures(tile);
	for(auto cre : guards)
	{
		vstd::amax(guardDanger, evaluateDanger(cre) * fh->getTacticalAdvantage(visitor, dynamic_cast<const CArmedInstance *>(cre))); //we are interested in strongest monster around
	}

	//TODO mozna odwiedzic blockvis nie ruszajac straznika
	return std::max(objectDanger, guardDanger);
}

ui64 evaluateDanger(const CGObjectInstance * obj)
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
	case Obj::GARRISON:
	case Obj::GARRISON2:
	{
		InfoAboutTown iat;
		cb->getTownInfo(obj, iat);
		return iat.army.getStrength();
	}
	case Obj::MONSTER:
	{
		//TODO!!!!!!!!
		const CGCreature * cre = dynamic_cast<const CGCreature *>(obj);
		return cre->getArmyStrength();
	}
	case Obj::CREATURE_GENERATOR1:
	case Obj::CREATURE_GENERATOR4:
	{
		const CGDwelling * d = dynamic_cast<const CGDwelling *>(obj);
		return d->getArmyStrength();
	}
	case Obj::MINE:
	case Obj::ABANDONED_MINE:
	{
		const CArmedInstance * a = dynamic_cast<const CArmedInstance *>(obj);
		return a->getArmyStrength();
	}
	case Obj::CRYPT: //crypt
	case Obj::CREATURE_BANK: //crebank
	case Obj::DRAGON_UTOPIA:
	case Obj::SHIPWRECK: //shipwreck
	case Obj::DERELICT_SHIP: //derelict ship
//	case Obj::PYRAMID:
		return fh->estimateBankDanger(dynamic_cast<const CBank *>(obj));
	case Obj::PYRAMID:
	{
		if(obj->subID == 0)
			return fh->estimateBankDanger(dynamic_cast<const CBank *>(obj));
		else
			return 0;
	}
	default:
		return 0;
	}
}

bool compareDanger(const CGObjectInstance * lhs, const CGObjectInstance * rhs)
{
	return evaluateDanger(lhs) < evaluateDanger(rhs);
}

ui64 analyzeDanger(HeroPtr h, crint3 tile)
{
	si64 heroStrength = h->getTotalStrength();
	si64 dangerStrength = evaluateDanger(tile, *h);
	si64 dif = dangerStrength - heroStrength / SAFE_ATTACK_CONSTANT;

	return std::max((si64)0, dif);
}

bool canBeEmbarkmentPoint(const TerrainTile * t, bool fromWater)
{
	// TODO: Such information should be provided by pathfinder
	// Tile must be free or with unoccupied boat
	if(!t->blocked)
	{
		return true;
	}
	else if(!fromWater) // do not try to board when in water sector
	{
		if(t->visitableObjects.size() == 1 && t->topVisitableId() == Obj::BOAT)
			return true;
	}
	return false;
}

int3 whereToExplore(HeroPtr h)
{
	TimeCheck tc("where to explore");
	int radius = h->getSightRadius();
	int3 hpos = h->visitablePos();

	try //check if nearby tiles allow us to reveal anything - this is quick
	{
		return ai->explorationBestNeighbour(hpos, radius, h);
	}
	catch(cannotFulfillGoalException & e)
	{
		//perform exhaustive search
		return ai->explorationNewPoint(h);
	}
}

bool isBlockedBorderGate(int3 tileToHit) //TODO: is that function needed? should be handled by pathfinder
{
	if(cb->getTile(tileToHit)->topVisitableId() != Obj::BORDER_GATE)
		return false;
	auto gate = dynamic_cast<const CGKeys *>(cb->getTile(tileToHit)->topVisitableObj());
	return !gate->wasMyColorVisited(ai->playerID);
}
bool isBlockVisitObj(const int3 & pos)
{
	if(auto obj = cb->getTopObj(pos))
	{
		if(obj->blockVisit) //we can't stand on that object
			return true;
	}

	return false;
}

bool hasReachableNeighbor(int3 pos, const CPathsInfo* paths, CCallback * cbp) {
	for (crint3 dir : int3::getDirs())
	{
		int3 tile = pos + dir;
		if (cbp->isInTheMap(tile) && paths->getPathInfo(tile)->reachable())
		{
			return true;
		}
	}

	return false;
}

int howManyTilesWillBeDiscovered(const int3 & pos, int radious, CCallback * cbp, const CPathsInfo* pathsInfo)
{
	//TODO: do not explore dead-end boundaries
	int ret = 0;
	for(int x = pos.x - radious; x <= pos.x + radious; x++)
	{
		for(int y = pos.y - radious; y <= pos.y + radious; y++)
		{
			int3 npos = int3(x, y, pos.z);
			if(cbp->isInTheMap(npos) && pos.dist2d(npos) - 0.5 < radious && !cbp->isVisible(npos))
			{
				if(hasReachableNeighbor(npos, pathsInfo, cbp))
					ret++;
			}
		}
	}

	return ret;
}

bool boundaryBetweenTwoPoints(int3 pos1, int3 pos2, CCallback * cbp) //determines if two points are separated by known barrier
{
	int xMin = std::min(pos1.x, pos2.x);
	int xMax = std::max(pos1.x, pos2.x);
	int yMin = std::min(pos1.y, pos2.y);
	int yMax = std::max(pos1.y, pos2.y);

	for(int x = xMin; x <= xMax; ++x)
	{
		for(int y = yMin; y <= yMax; ++y)
		{
			int3 tile = int3(x, y, pos1.z); //use only on same level, ofc
			if(std::abs(pos1.dist2d(tile) - pos2.dist2d(tile)) < 1.5)
			{
				if(!(cbp->isVisible(tile) && cbp->getTile(tile)->blocked)) //if there's invisible or unblocked tile between, it's good
					return false;
			}
		}
	}
	return true; //if all are visible and blocked, we're at dead end
}

void getVisibleNeighbours(const std::vector<int3> & tiles, std::vector<int3> & out)
{
	for(const int3 & tile : tiles)
	{
		foreach_neighbour(tile, [&](int3 neighbour)
		{
			if(cb->isVisible(neighbour))
				out.push_back(neighbour);
		});
	}
}

bool compareHeroStrength(HeroPtr h1, HeroPtr h2)
{
	return h1->getTotalStrength() < h2->getTotalStrength();
}

bool isLevelHigher(HeroPtr h1, HeroPtr h2)
{
	return h1->exp > h2->exp;
}

bool compareArmyStrength(const CArmedInstance * a1, const CArmedInstance * a2)
{
	return a1->getArmyStrength() < a2->getArmyStrength();
}

bool compareArtifacts(const CArtifactInstance * a1, const CArtifactInstance * a2)
{
	auto art1 = a1->artType;
	auto art2 = a2->artType;

	if(art1->price == art2->price)
		return art1->valOfBonuses(Bonus::PRIMARY_SKILL) > art2->valOfBonuses(Bonus::PRIMARY_SKILL);
	else if(art1->price > art2->price)
		return true;
	else
		return false;
}

ui32 distanceToTile(const CGHeroInstance* hero, int3 pos) {
	auto pathsInfo = cb->getPathsInfo(hero);

	return distanceToTile(pathsInfo, pos);
}

ui32 distanceToTile(const CPathsInfo* pathsInfo, int3 pos) {
	auto pathInfo = pathsInfo->getPathInfo(pos);
	auto hero = pathsInfo->hero;
	int result = (int)(pathInfo->turns * hero->maxMovePoints(true) + hero->movement) - (int)pathInfo->moveRemains;

	return std::max(result, 0);
}

const CGHeroInstance* getNearestHero(std::vector<const CGHeroInstance*> heroes, int3 pos) {
	const CGHeroInstance* carrier = heroes.front();
	ui32 optimalDistance = distanceToTile(carrier, pos);

	for (auto hero : heroes) {
		int newDistance = distanceToTile(hero, pos);

		if (newDistance < optimalDistance) {
			carrier = hero;
			optimalDistance = newDistance;
		}
	}

	return carrier;
}