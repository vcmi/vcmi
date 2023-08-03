/*
* CollectRes.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Goals.h"
#include "../VCAI.h"
#include "../AIUtility.h"
#include "../AIhelper.h"
#include "../FuzzyHelper.h"
#include "../ResourceManager.h"
#include "../BuildingManager.h"
#include "../../../lib/mapObjects/CGMarket.h"
#include "../../../lib/StringConstants.h"


extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool CollectRes::operator==(const CollectRes & other) const
{
	return resID == other.resID;
}

TGoalVec CollectRes::getAllPossibleSubgoals()
{
	TGoalVec ret;

	auto givesResource = [this](const CGObjectInstance * obj) -> bool
	{
		//TODO: move this logic to object side
		//TODO: remember mithril exists
		//TODO: water objects
		//TODO: Creature banks

		//return false first from once-visitable, before checking if they were even visited
		switch (obj->ID.num)
		{
		case Obj::TREASURE_CHEST:
			return resID == GameResID(EGameResID::GOLD);
			break;
		case Obj::RESOURCE:
			return obj->subID == resID;
			break;
		case Obj::MINE:
			return (obj->subID == resID &&
				(cb->getPlayerRelations(obj->tempOwner, ai->playerID) == PlayerRelations::ENEMIES)); //don't capture our mines
			break;
		case Obj::CAMPFIRE:
			return true; //contains all resources
			break;
		case Obj::WINDMILL:
			switch (GameResID(resID).toEnum())
			{
			case EGameResID::GOLD:
			case EGameResID::WOOD:
				return false;
			}
			break;
		case Obj::WATER_WHEEL:
			if (resID != GameResID(EGameResID::GOLD))
				return false;
			break;
		case Obj::MYSTICAL_GARDEN:
			if ((resID != GameResID(EGameResID::GOLD)) && (resID != GameResID(EGameResID::GEMS)))
				return false;
			break;
		case Obj::LEAN_TO:
		case Obj::WAGON:
			if (resID != GameResID(EGameResID::GOLD))
				return false;
			break;
		default:
			return false;
			break;
		}
		return !vstd::contains(ai->alreadyVisited, obj); //for weekly / once visitable
	};

	std::vector<const CGObjectInstance *> objs;
	for (auto obj : ai->visitableObjs)
	{
		if (givesResource(obj))
			objs.push_back(obj);
	}
	for (auto h : cb->getHeroesInfo())
	{
		std::vector<const CGObjectInstance *> ourObjs(objs); //copy common objects

		for (auto obj : ai->reservedHeroesMap[h]) //add objects reserved by this hero
		{
			if (givesResource(obj))
				ourObjs.push_back(obj);
		}

		for (auto obj : ourObjs)
		{
			auto waysToGo = ai->ah->howToVisitObj(h, ObjectIdRef(obj));

			vstd::concatenate(ret, waysToGo);
		}
	}
	return ret;
}

TSubgoal CollectRes::whatToDoToAchieve()
{
	auto goals = getAllPossibleSubgoals();
	auto trade = whatToDoToTrade();
	if (!trade->invalid())
		goals.push_back(trade);

	if (goals.empty())
		return sptr(Explore()); //we can always do that
	else
		return fh->chooseSolution(goals); //TODO: evaluate trading
}

TSubgoal CollectRes::whatToDoToTrade()
{
	std::vector<const IMarket *> markets;

	std::vector<const CGObjectInstance *> visObjs;
	ai->retrieveVisitableObjs(visObjs, true);
	for(const CGObjectInstance * obj : visObjs)
	{
		if(const IMarket * m = IMarket::castFrom(obj, false); m && m->allowsTrade(EMarketMode::RESOURCE_RESOURCE))
		{
			if(obj->ID == Obj::TOWN)
			{
				if(obj->tempOwner == ai->playerID)
					markets.push_back(m);
			}
			else
				markets.push_back(m);
		}
	}

	boost::sort(markets, [](const IMarket * m1, const IMarket * m2) -> bool
	{
		return m1->getMarketEfficiency() < m2->getMarketEfficiency();
	});

	markets.erase(boost::remove_if(markets, [](const IMarket * market) -> bool
	{
		auto * o = dynamic_cast<const CGObjectInstance *>(market);
		if(o && !(o->ID == Obj::TOWN && o->tempOwner == ai->playerID))
		{
			if(!ai->isAccessible(o->visitablePos()))
				return true;
		}
		return false;
	}), markets.end());

	if (!markets.size())
	{
		for (const CGTownInstance * t : cb->getTownsInfo())
		{
			if (cb->canBuildStructure(t, BuildingID::MARKETPLACE) == EBuildingState::ALLOWED)
				return sptr(BuildThis(BuildingID::MARKETPLACE, t).setpriority(2));
		}
	}
	else
	{
		const IMarket * m = markets.back();
		//attempt trade at back (best prices)
		int howManyCanWeBuy = 0;
		for (GameResID i = EGameResID::WOOD; i <= EGameResID::GOLD; ++i)
		{
			if (GameResID(i) == resID)
				continue;
			int toGive = -1, toReceive = -1;
			m->getOffer(GameResID(i), resID, toGive, toReceive, EMarketMode::RESOURCE_RESOURCE);
			assert(toGive > 0 && toReceive > 0);
			howManyCanWeBuy += toReceive * (ai->ah->freeResources()[i] / toGive);
		}

		if (howManyCanWeBuy >= value)
		{
			auto * o = dynamic_cast<const CGObjectInstance *>(m);
			auto backObj = cb->getTopObj(o->visitablePos()); //it'll be a hero if we have one there; otherwise marketplace
			assert(backObj);
			auto objid = o->id.getNum();
			if (backObj->tempOwner != ai->playerID) //top object not owned
			{
				return sptr(VisitObj(objid)); //just go there
			}
			else //either it's our town, or we have hero there
			{
				return sptr(Trade(static_cast<EGameResID>(resID), value, objid).setisElementar(true)); //we can do this immediately
			}
		}
	}
	return sptr(Invalid()); //cannot trade
}

bool CollectRes::fulfillsMe(TSubgoal goal)
{
	if (goal->resID == resID)
		if (goal->value >= value)
			return true;

	return false;
}
