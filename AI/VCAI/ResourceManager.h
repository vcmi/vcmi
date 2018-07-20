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

struct DLL_EXPORT ResourceObjective
{
	ResourceObjective(TResources &res, Goals::TSubgoal goal);
	bool operator < (const ResourceObjective &ro);

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

public:
	bool canAfford(const TResources & cost) const;
	TResources freeResources() const; //owned resources minus gold reserve
	TResource freeGold() const; //owned resources minus gold reserve
	TResources estimateIncome() const;

	void reserveResoures(TResources &res, Goals::TSubgoal goal = Goals::TSubgoal());
	Goals::TSubgoal whatToDo(); //pop highest-priority goal
	Goals::TSubgoal whatToDo(TResources &res, Goals::TSubgoal goal); //can we afford this goal or need to CollectRes?
	bool notifyGoalCompleted(Goals::TSubgoal goal);
	bool updateGoal(Goals::TSubgoal goal); //new goal must have same properties but different priority

private:
	TResources saving;

	boost::heap::priority_queue<ResourceObjective> queue;

	//TODO: register?
	template<typename Handler> void serializeInternal(Handler & h, const int version)
	{
		h & saving;
		h & queue;
	}
};