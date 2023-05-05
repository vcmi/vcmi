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
#include "AIGateway.h"
#include "Goals/Goals.h"

#include "../../lib/UnlockGuard.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/mapObjects/MapObjects.h"
#include "../../lib/mapping/CMapDefines.h"

#include "../../lib/GameSettings.h"

#include <vcmi/CreatureService.h>

namespace NKAI
{

extern boost::thread_specific_ptr<AIGateway> ai;

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
		//const bool owned = obj && obj->tempOwner == ai->playerID;

		if(doWeExpectNull && !obj)
		{
			return nullptr;
		}
		else
		{
			if (!obj)
				logAi->error("Accessing no longer accessible hero %s!", h->getNameTranslated());
			//assert(obj);
			//assert(owned);
		}
	}

	return h;
}

const CGHeroInstance * HeroPtr::get(CCallback * cb, bool doWeExpectNull) const
{
	//TODO? check if these all assertions every time we get info about hero affect efficiency
	//
	//behave terribly when attempting unauthorized access to hero that is not ours (or was lost)
	assert(doWeExpectNull || h);

	if(h)
	{
		auto obj = cb->getObj(hid);
		//const bool owned = obj && obj->tempOwner == ai->playerID;

		if(doWeExpectNull && !obj)
		{
			return nullptr;
		}
		else
		{
			assert(obj);
			//assert(owned);
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

bool isSafeToVisit(HeroPtr h, const CCreatureSet * heroArmy, uint64_t dangerStrength)
{
	const ui64 heroStrength = h->getFightingStrength() * heroArmy->getArmyStrength();

	if(dangerStrength)
	{
		return heroStrength / SAFE_ATTACK_CONSTANT > dangerStrength;
	}

	return true; //there's no danger
}

bool isSafeToVisit(HeroPtr h, uint64_t dangerStrength)
{
	return isSafeToVisit(h, h.get(), dangerStrength);
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

bool isObjectPassable(const Nullkiller * ai, const CGObjectInstance * obj)
{
	return isObjectPassable(obj, ai->playerID, ai->cb->getPlayerRelations(obj->tempOwner, ai->playerID));
}

bool isObjectPassable(const CGObjectInstance * obj)
{
	return isObjectPassable(obj, ai->playerID, ai->myCb->getPlayerRelations(obj->tempOwner, ai->playerID));
}

// Pathfinder internal helper
bool isObjectPassable(const CGObjectInstance * obj, PlayerColor playerColor, PlayerRelations::PlayerRelations objectRelations)
{
	if((obj->ID == Obj::GARRISON || obj->ID == Obj::GARRISON2)
		&& objectRelations != PlayerRelations::ENEMIES)
		return true;

	if(obj->ID == Obj::BORDER_GATE)
	{
		auto quest = dynamic_cast<const CGKeys *>(obj);

		if(quest->passableFor(playerColor))
			return true;
	}

	return false;
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
		ci.cre = VLC->creatures()->getById(ci.creID);
		ci.level = ci.cre->getLevel(); //this is creature tier, while tryRealize expects dwelling level. Ignore.
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
		return art1->valOfBonuses(BonusType::PRIMARY_SKILL) > art2->valOfBonuses(BonusType::PRIMARY_SKILL);
	else
		return art1->price > art2->price;
}

bool isWeeklyRevisitable(const CGObjectInstance * obj)
{
	if(!obj)
		return false;

	//TODO: allow polling of remaining creatures in dwelling
	if(const auto * rewardable = dynamic_cast<const CRewardableObject *>(obj))
		return rewardable->configuration.getResetDuration() == 7;

	if(dynamic_cast<const CGDwelling *>(obj))
		return true;
	if(dynamic_cast<const CBank *>(obj)) //banks tend to respawn often in mods
		return true;

	switch(obj->ID)
	{
	case Obj::STABLES:
	case Obj::MAGIC_WELL:
	case Obj::HILL_FORT:
		return true;
	case Obj::BORDER_GATE:
	case Obj::BORDERGUARD:
		return (dynamic_cast<const CGKeys *>(obj))->wasMyColorVisited(ai->playerID); //FIXME: they could be revisited sooner than in a week
	}
	return false;
}

uint64_t timeElapsed(std::chrono::time_point<std::chrono::high_resolution_clock> start)
{
	auto end = std::chrono::high_resolution_clock::now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

// todo: move to obj manager
bool shouldVisit(const Nullkiller * ai, const CGHeroInstance * h, const CGObjectInstance * obj)
{
	auto relations = ai->cb->getPlayerRelations(obj->tempOwner, h->tempOwner);

	switch(obj->ID)
	{
	case Obj::TOWN:
	case Obj::HERO: //never visit our heroes at random
		return relations == PlayerRelations::ENEMIES; //do not visit our towns at random

	case Obj::BORDER_GATE:
	{
		for(auto q : ai->cb->getMyQuests())
		{
			if(q.obj == obj)
			{
				return false; // do not visit guards or gates when wandering
			}
		}
		return true; //we don't have this quest yet
	}
	case Obj::BORDERGUARD: //open borderguard if possible
		return (dynamic_cast<const CGKeys *>(obj))->wasMyColorVisited(ai->playerID);
	case Obj::SEER_HUT:
	{
		for(auto q : ai->cb->getMyQuests())
		{
			if(q.obj == obj)
			{
				if(q.quest->checkQuest(h))
					return true; //we completed the quest
				else
					return false; //we can't complete this quest
			}
		}
		return true; //we don't have this quest yet
	}
	case Obj::CREATURE_GENERATOR1:
	{
		if(relations == PlayerRelations::ENEMIES)
			return true; //flag just in case

		if(relations == PlayerRelations::ALLIES)
			return false;

		const CGDwelling * d = dynamic_cast<const CGDwelling *>(obj);

		for(auto level : d->creatures)
		{
			for(auto c : level.second)
			{
				if(level.first
					&& h->getSlotFor(CreatureID(c)) != SlotID()
					&& ai->cb->getResourceAmount().canAfford(c.toCreature()->getFullRecruitCost()))
				{
					return true;
				}
			}
		}

		return false;
	}
	case Obj::HILL_FORT:
	{
		for(auto slot : h->Slots())
		{
			if(slot.second->type->hasUpgrades())
				return true; //TODO: check price?
		}
		return false;
	}
	case Obj::MONOLITH_ONE_WAY_ENTRANCE:
	case Obj::MONOLITH_ONE_WAY_EXIT:
	case Obj::MONOLITH_TWO_WAY:
	case Obj::WHIRLPOOL:
		return false;
	case Obj::SCHOOL_OF_MAGIC:
	case Obj::SCHOOL_OF_WAR:
	{
		if(ai->getFreeGold() < 1000)
			return false;
		break;
	}
	case Obj::LIBRARY_OF_ENLIGHTENMENT:
		if(h->level < 10)
			return false;
		break;
	case Obj::TREE_OF_KNOWLEDGE:
	{
		if(ai->heroManager->getHeroRole(h) == HeroRole::SCOUT)
			return false;

		TResources myRes = ai->getFreeResources();
		if(myRes[EGameResID::GOLD] < 2000 || myRes[EGameResID::GEMS] < 10)
			return false;
		break;
	}
	case Obj::MAGIC_WELL:
		return h->mana < h->manaLimit();
	case Obj::PRISON:
		return ai->cb->getHeroesInfo().size() < VLC->settings()->getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP);
	case Obj::TAVERN:
	case Obj::EYE_OF_MAGI:
	case Obj::BOAT:
	case Obj::SIGN:
		return false;
	}

	if(obj->wasVisited(h)) //it must pointer to hero instance, heroPtr calls function wasVisited(ui8 player);
		return false;

	return true;
}

bool townHasFreeTavern(const CGTownInstance * town)
{
	if(!town->hasBuilt(BuildingID::TAVERN)) return false;
	if(!town->visitingHero) return true;

	bool canMoveVisitingHeroToGarnison = !town->getUpperArmy()->stacksCount();

	return canMoveVisitingHeroToGarnison;
}

}
