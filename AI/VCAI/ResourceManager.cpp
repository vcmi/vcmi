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

#define GOLD_RESERVE (10000); //at least we'll be able to reach capitol

ResourceManager * rm;

ResourceObjective::ResourceObjective(TResources & Res, Goals::TSubgoal Goal)
	: resources(Res), goal(Goal)
{
}

//bool operator<(const ResourceObjective & lhs, const ResourceObjective & rhs)
//{
//	return lhs.goal->priority < rhs.goal->priority;
//}
//bool operator<(const ResourceObjective lhs, const ResourceObjective rhs)
//{
//	return lhs.goal->priority < rhs.goal->priority;
//}

bool ResourceObjective::operator<(const ResourceObjective & ro) const
{
	return goal->priority < ro.goal->priority;
}

ResourceManager::ResourceManager(CPlayerSpecificInfoCallback * CB)
	: ai(nullptr)
{
	cb = CB;
}

void ResourceManager::setCB(CPlayerSpecificInfoCallback * CB)
{
	cb = CB;
}

void ResourceManager::setAI(VCAI * AI)
{
	ai = AI;
}

bool ResourceManager::canAfford(const TResources & cost) const
{
	return freeResources().canAfford(cost);
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

void ResourceManager::reserveResoures(TResources & res, Goals::TSubgoal goal)
{
	if (!goal->invalid())
		queue.push(ResourceObjective(res, goal));
	else
		logAi->warn("Attempt to reserve resources for Invalid goal");
}

Goals::TSubgoal ResourceManager::whatToDo()
{
	if (queue.size()) //TODO: check if we can afford. if not, then CollectRes
	{
		auto o = queue.top();
		if (o.resources <= freeResources())
			return o.goal;
		else
		{
			auto income = estimateIncome();
			Res::ERes resourceType = Res::INVALID; //TODO: unit test
			TResource amountToCollect = 0;

			typedef std::pair<Res::ERes, TResource> resPair;
			std::map<Res::ERes, TResource> missingResources;
			for (auto it = Res::ResourceSet::nziterator(o.resources); it.valid(); it++)
				missingResources[it->resType] = it->resVal;
			//TODO: sum missing resources of given type for ALL reserved objectives

			for (const resPair & p: missingResources)
			{
				if (!income[p.first]) //prioritize resources with 0 income
				{
					resourceType = p.first;
					amountToCollect = p.second;
					break;
				}
			}
			if (resourceType == Res::INVALID) //no needed resources has 0 income, 
			{
				//find the one which takes longest to collect
				auto incomeComparer = [&income](const resPair & lhs, const resPair & rhs) -> bool
				{
					return ((float)lhs.second / income[lhs.first]) < ((float)rhs.second / income[rhs.second]);
					//theoretically income can be negative, but that falls into this comparison
				};

				resourceType = boost::max_element(missingResources, incomeComparer)->first;
				amountToCollect = missingResources[resourceType];
			}
			return Goals::sptr(Goals::CollectRes(resourceType, amountToCollect));
		}
	}
	else
		return Goals::sptr(Goals::Invalid()); //nothing else to do
}

Goals::TSubgoal ResourceManager::whatToDo(TResources &res, Goals::TSubgoal goal)
{
	TResources accumulatedResources;

	ResourceObjective ro(res, goal);
	queue.push(ro);
	for (auto it : queue) //check if we can afford all the objectives with higher priority
	{
		accumulatedResources += it.resources;
		if (accumulatedResources > freeResources()) //can't afford
			return whatToDo(); 
		else //can afford all goals up to this point
		{
			if (it.goal == goal)
				return goal; //can afford immediately
		}
	}
	return whatToDo(); //should return CollectRes for highest-priority goal

	//TODO: consider returning CollectRes for the goal requested (in case it needs different resources than our priority)
}

bool ResourceManager::notifyGoalCompleted(Goals::TSubgoal goal)
{
	if (goal->invalid())
		logAi->warn("Attempt to complete Invalid goal");

	bool removedGoal = false;
	while (true)
	{ //unfortunatelly we can't use remove_if on heap
		auto it = boost::find_if(queue, [goal](const ResourceObjective & ro) -> bool
		{
			return *ro.goal == *goal || goal->fulfillsMe (goal);
		});
		if (it != queue.end()) //removed at least one
		{
			queue.erase(queue.s_handle_from_iterator(it));
			logAi->debug("Removed goal %s from ResourceManager.", goal->name());
			removedGoal = true;
		}
		else //found nothing more to remove
			return removedGoal;
	}
	return removedGoal;
}

bool ResourceManager::updateGoal(Goals::TSubgoal goal)
{
	//we update priority of goal if it is easier or more difficult to complete
	if (goal->invalid())
		logAi->warn("Attempt to update Invalid goal");

	auto it = boost::find_if(queue, [goal](const ResourceObjective & ro) -> bool
	{
		return ro.goal == goal;
	});
	if (it != queue.end())
	{
		it->goal->setpriority(goal->priority);
		auto handle = queue.s_handle_from_iterator(it);
		queue.update(handle); //restore order
		return true;
	}
	else
		return false;
}

bool ResourceManager::hasTasksLeft()
{
	return !queue.empty();
}

TResources ResourceManager::reservedResources() const
{
	TResources res;
	for (auto it : queue) //substract the value of reserved goals
		res += it.resources;
	return res;
}

TResources ResourceManager::freeResources() const
{
	TResources myRes = cb->getResourceAmount();
	auto towns = cb->getTownsInfo();
	if (towns.size()) //we don't save for Capitol if there are no towns
	{
		if (std::none_of(towns.begin(), towns.end(), [](const CGTownInstance * x) -> bool
		{
			return x->builtBuildings.find(BuildingID::CAPITOL) != x->builtBuildings.end();
		}))
		{
			myRes[Res::GOLD] -= GOLD_RESERVE;
			//what if capitol is blocked from building in all possessed towns (set in map editor)?
		}
	}
	myRes -= reservedResources(); //substract the value of reserved goals

	for (auto & val : myRes)
		vstd::amax(val, 0); //never negative

	return myRes;
}

TResource ResourceManager::freeGold() const
{
	return freeResources()[Res::GOLD];
}
