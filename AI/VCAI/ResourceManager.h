/*
* ResourceManager.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "AIUtility.h"
#include "Goals.h"
#include "../../lib/GameConstants.h"
#include "../../lib/VCMI_Lib.h"
#include "VCAI.h"
#include <boost/heap/binomial_heap.hpp>

struct DLL_EXPORT ResourceObjective
{
	ResourceObjective() = default;
	ResourceObjective(TResources &res, Goals::TSubgoal goal);
	bool operator < (const ResourceObjective &ro) const;

	TResources resources; //how many resoures do we need
	Goals::TSubgoal goal; //what for (build, gather army etc...)

	 //TODO: register?
	template<typename Handler> void serializeInternal(Handler & h, const int version)
	{
		h & resources;
		//h & goal; //FIXME: goal serialization is broken
	}
};

class DLL_EXPORT ResourceManager //: public: IAIManager
{
	/*Resource Manager is a smart shopping list for AI to help
	Uses priority queue based on CGoal.priority */
	friend class VCAI;
	friend struct SetGlobalState;

	CPlayerSpecificInfoCallback * cb; //this is enough, but we downcast from CCallback
	VCAI * ai;

public:
	ResourceManager() = default;
	ResourceManager(CPlayerSpecificInfoCallback * CB, VCAI * AI = nullptr); //for tests only

	bool canAfford(const TResources & cost) const;
	TResources reservedResources() const;
	TResources freeResources() const; //owned resources minus reserve
	TResource freeGold() const;
	TResources estimateIncome() const;

	void reserveResoures(TResources &res, Goals::TSubgoal goal = Goals::TSubgoal());
	Goals::TSubgoal collectResourcesForOurGoal(ResourceObjective &o) const;
	Goals::TSubgoal whatToDo(); //pop highest-priority goal
	Goals::TSubgoal whatToDo(TResources &res, Goals::TSubgoal goal); //can we afford this goal or need to CollectRes?
	bool notifyGoalCompleted(Goals::TSubgoal goal);
	bool updateGoal(Goals::TSubgoal goal); //new goal must have same properties but different priority
	bool tryPush(ResourceObjective &o);
	bool hasTasksLeft();

	//for unit tests. shoudl be private.


private:
	TResources saving;

	boost::heap::binomial_heap<ResourceObjective> queue;

	void setCB(CPlayerSpecificInfoCallback * CB);
	void setAI(VCAI * AI);

	//TODO: register?
	template<typename Handler> void serializeInternal(Handler & h, const int version)
	{
		h & saving;
		h & queue;
	}
};