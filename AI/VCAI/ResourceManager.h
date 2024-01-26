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
#include "../../lib/GameConstants.h"
#include "../../lib/VCMI_Lib.h"
#include "VCAI.h"
#include <boost/heap/binomial_heap.hpp>

class AIhelper;
class IResourceManager;

struct DLL_EXPORT ResourceObjective
{
	ResourceObjective() = default;
	ResourceObjective(const TResources &res, Goals::TSubgoal goal);
	bool operator < (const ResourceObjective &ro) const;

	TResources resources; //how many resoures do we need
	Goals::TSubgoal goal; //what for (build, gather army etc...)

	 //TODO: register?
	template<typename Handler> void serializeInternal(Handler & h)
	{
		h & resources;
		//h & goal; //FIXME: goal serialization is broken
	}
};

class DLL_EXPORT IResourceManager //: public: IAbstractManager
{
public:
	virtual ~IResourceManager() = default;
	virtual void init(CPlayerSpecificInfoCallback * CB) = 0;
	virtual void setAI(VCAI * AI) = 0;

	virtual TResources reservedResources() const = 0;
	virtual TResources freeResources() const = 0;
	virtual TResource freeGold() const = 0;
	virtual TResources allResources() const = 0;
	virtual TResource allGold() const = 0;

	virtual Goals::TSubgoal whatToDo() const = 0;//get highest-priority goal
	virtual Goals::TSubgoal whatToDo(TResources &res, Goals::TSubgoal goal) = 0;
	virtual bool containsObjective(Goals::TSubgoal goal) const = 0;
	virtual bool hasTasksLeft() const = 0;
	virtual bool removeOutdatedObjectives(std::function<bool(const Goals::TSubgoal &)> predicate) = 0; //remove ResourceObjectives from queue if ResourceObjective->goal meets specific criteria
	virtual bool notifyGoalCompleted(Goals::TSubgoal goal) = 0;
};

class DLL_EXPORT ResourceManager : public IResourceManager
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
	TResources allResources() const override;
	TResource allGold() const override;

	Goals::TSubgoal whatToDo() const override; //peek highest-priority goal
	Goals::TSubgoal whatToDo(TResources & res, Goals::TSubgoal goal) override; //can we afford this goal or need to CollectRes?
	bool containsObjective(Goals::TSubgoal goal) const override;
	bool hasTasksLeft() const override;
	bool removeOutdatedObjectives(std::function<bool(const Goals::TSubgoal &)> predicate) override;
	bool notifyGoalCompleted(Goals::TSubgoal goal) override;

protected: //not-const actions only for AI
	virtual void reserveResoures(const TResources & res, Goals::TSubgoal goal = Goals::TSubgoal());
	virtual bool updateGoal(Goals::TSubgoal goal); //new goal must have same properties but different priority
	virtual bool tryPush(const ResourceObjective &o);

	//inner processing
	virtual TResources estimateIncome() const;
	virtual Goals::TSubgoal collectResourcesForOurGoal(ResourceObjective &o) const;

	void init(CPlayerSpecificInfoCallback * CB) override;
	void setAI(VCAI * AI) override;

private:
	TResources saving;

	boost::heap::binomial_heap<ResourceObjective> queue;

	void dumpToLog() const;

	//TODO: register?
	template<typename Handler> void serializeInternal(Handler & h)
	{
		h & saving;
		h & queue;
	}
};
