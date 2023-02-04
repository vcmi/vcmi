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
#include "FuzzyHelper.h"
#include "Goals/Goals.h"

#include "../../lib/UnlockGuard.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/mapObjects/CBank.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CQuest.h"
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

ObjectIdRef::operator bool() const
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
	name = h->getNameTranslated();
	hid = H->id;
//	infosCount[ai->playerID][hid]++;
}

HeroPtr::HeroPtr()
{
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

bool HeroPtr::operator==(const HeroPtr & rhs) const
{
	return h == rhs.get(true);
}

bool CDistanceSorter::operator()(const CGObjectInstance * lhs, const CGObjectInstance * rhs) const
{
	const CGPathNode * ln = ai->myCb->getPathsInfo(hero)->getPathInfo(lhs->visitablePos());
	const CGPathNode * rn = ai->myCb->getPathsInfo(hero)->getPathInfo(rhs->visitablePos());

	return ln->getCost() < rn->getCost();
}

bool isSafeToVisit(HeroPtr h, crint3 tile)
{
	return isSafeToVisit(h, fh->evaluateDanger(tile, h.get()));
}

bool isSafeToVisit(HeroPtr h, uint64_t dangerStrength)
{
	const ui64 heroStrength = h->getTotalStrength();

	if(dangerStrength)
	{
		return heroStrength / SAFE_ATTACK_CONSTANT > dangerStrength;
	}

	return true; //there's no danger
}

bool isObjectRemovable(const CGObjectInstance * obj)
{
	//FIXME: move logic to object property!
	switch (obj->ID)
	{
	case Obj::MONSTER:
	case Obj::RESOURCE:
	case Obj::CAMPFIRE:
	case Obj::TREASURE_CHEST:
	case Obj::ARTIFACT:
	case Obj::BORDERGUARD:
	case Obj::FLOTSAM:
	case Obj::PANDORAS_BOX:
	case Obj::OCEAN_BOTTLE:
	case Obj::SEA_CHEST:
	case Obj::SHIPWRECK_SURVIVOR:
	case Obj::SPELL_SCROLL:
		return true;
		break;
	default:
		return false;
		break;
	}

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

bool isBlockedBorderGate(int3 tileToHit) //TODO: is that function needed? should be handled by pathfinder
{
	if(cb->getTile(tileToHit)->topVisitableId() != Obj::BORDER_GATE)
		return false;
	auto gate = dynamic_cast<const CGKeys *>(cb->getTile(tileToHit)->topVisitableObj());
	return !gate->passableFor(ai->playerID);
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

creInfo infoFromDC(const dwellingContent & dc)
{
	creInfo ci;
	ci.count = dc.first;
	ci.creID = dc.second.size() ? dc.second.back() : CreatureID(-1); //should never be accessed
	if (ci.creID != -1)
	{
		ci.cre = VLC->creh->objects[ci.creID];
		ci.level = ci.cre->level; //this is cretaure tier, while tryRealize expects dwelling level. Ignore.
	}
	else
	{
		ci.cre = nullptr;
		ci.level = 0;
	}
	return ci;
}

bool compareHeroStrength(HeroPtr h1, HeroPtr h2)
{
	return h1->getTotalStrength() < h2->getTotalStrength();
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
	else
		return art1->price > art2->price;
}
