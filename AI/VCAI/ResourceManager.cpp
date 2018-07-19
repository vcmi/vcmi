/*
* ResourceManager.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ResourceManager.h"

#include "../../CCallback.h"
#include "../../lib/mapObjects/MapObjects.h"

ResourceManager * rm;

const int GOLD_RESERVE = 10000; //when buying creatures we want to keep at least this much gold (10000 so at least we'll be able to reach capitol)

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

bool ResourceManager::containsSavedRes(const TResources & cost) const
{
	for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
	{
		if (saving[i] && cost[i])
			return true;
	}

	return false;
}

TResources ResourceManager::estimateIncome() const
{
	TResources ret;
	for (const CGTownInstance * t : cb->getTownsInfo())
	{
		ret += t->dailyIncome();
	}

	for (const CGObjectInstance * obj : ai->getFlaggedObjects())
	{
		if (obj->ID == Obj::MINE)
		{
			switch (obj->subID)
			{
			case Res::WOOD:
			case Res::ORE:
				ret[obj->subID] += WOOD_ORE_MINE_PRODUCTION;
				break;
			case Res::GOLD:
			case 7: //abandoned mine -> also gold
				ret[Res::GOLD] += GOLD_MINE_PRODUCTION;
				break;
			default:
				ret[obj->subID] += RESOURCE_MINE_PRODUCTION;
				break;
			}
		}
	}

	return ret;
}

TResources ResourceManager::freeResources() const
{
	TResources myRes = cb->getResourceAmount();
	auto iterator = cb->getTownsInfo();
	if (std::none_of(iterator.begin(), iterator.end(), [](const CGTownInstance * x) -> bool
	{
		return x->builtBuildings.find(BuildingID::CAPITOL) != x->builtBuildings.end();
	}))
	{
		myRes[Res::GOLD] -= GOLD_RESERVE;
		//what if capitol is blocked from building in all possessed towns (set in map editor)?
		//What about reserve for city hall or something similar in that case?
	}
	vstd::amax(myRes[Res::GOLD], 0);
	return myRes;
}