/*
* AIhelper.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

/*
	!!!	Note: Include THIS file at the end of include list to avoid "undefined base class" error
*/

#include "ResourceManager.h"

class ResourceManager;

//indirection interface for various modules
class DLL_EXPORT AIhelper : public IResourceManager
{
	friend class VCAI;
	friend struct SetGlobalState; //mess?

	//members are thread_specific. AIhelper is global
	std::shared_ptr<ResourceManager> resourceManager;
	std::shared_ptr<VCAI> ai;
public:
	AIhelper();
	~AIhelper();

	//TODO: consider common interface with Resource Manager?
	bool canAfford(const TResources & cost) const;
	TResources reservedResources() const override;
	TResources freeResources() const override;
	TResource freeGold() const override;
	TResources allResources() const override;
	TResource allGold() const override;

	Goals::TSubgoal whatToDo(TResources &res, Goals::TSubgoal goal) override;
	Goals::TSubgoal whatToDo() const override; //peek highest-priority goal
	bool hasTasksLeft() const override;

private:
	bool notifyGoalCompleted(Goals::TSubgoal goal);

	void setCB(CPlayerSpecificInfoCallback * CB) override;
	void setAI(VCAI * AI) override;
};

