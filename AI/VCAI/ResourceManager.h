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

class AIhelper;
class IResourceManager;

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

class IResourceManager
{
public:
	virtual ~IResourceManager() = default;
	virtual void setCB(CPlayerSpecificInfoCallback * CB) = 0;
	virtual void setAI(VCAI * AI) = 0;

	virtual TResources reservedResources() const = 0;
	virtual TResources freeResources() const = 0;
	virtual TResource freeGold() const = 0;

	virtual Goals::TSubgoal whatToDo() const = 0;//get highest-priority goal
	virtual Goals::TSubgoal whatToDo(TResources &res, Goals::TSubgoal goal) = 0;
	virtual bool hasTasksLeft() const = 0;
private:
	virtual bool notifyGoalCompleted(Goals::TSubgoal goal) = 0;
};

class DLL_EXPORT ResourceManager : public IResourceManager//: public: IAIManager
{
	/*Resource Manager is a smart shopping list for AI to help
	Uses priority queue based on CGoal.priority */
	friend class VCAI;
	friend class AIhelper;
	friend struct SetGlobalState;

	CPlayerSpecificInfoCallback * cb; //this is enough, but we downcast from CCallback
	VCAI * ai;

public:
	ResourceManager() = default;
	ResourceManager(CPlayerSpecificInfoCallback * CB, VCAI * AI = nullptr); //for tests only

	bool canAfford(const TResources & cost) const;
	TResources reservedResources() const override;
	TResources freeResources() const override;
	TResource freeGold() const override;

	Goals::TSubgoal whatToDo() const override; //pop highest-priority goal
	Goals::TSubgoal whatToDo(TResources &res, Goals::TSubgoal goal); //can we afford this goal or need to CollectRes?
	bool hasTasksLeft() const override;

protected: //not-const actions only for AI
	void reserveResoures(TResources &res, Goals::TSubgoal goal = Goals::TSubgoal());
	bool notifyGoalCompleted(Goals::TSubgoal goal);
	bool updateGoal(Goals::TSubgoal goal); //new goal must have same properties but different priority
	bool tryPush(ResourceObjective &o);

	//inner processing
	TResources estimateIncome() const;
	Goals::TSubgoal collectResourcesForOurGoal(ResourceObjective &o) const;

	void setCB(CPlayerSpecificInfoCallback * CB) override;
	void setAI(VCAI * AI) override;

private:
	TResources saving;

	boost::heap::binomial_heap<ResourceObjective> queue;

	//TODO: register?
	template<typename Handler> void serializeInternal(Handler & h, const int version)
	{
		h & saving;
		h & queue;
	}
};