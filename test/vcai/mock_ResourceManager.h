#pragma once

#include "gtest/gtest.h"
#include "../AI/VCAI/ResourceManager.h"

class ResourceManager;
class CPlayerSpecificInfoCallback;
class VCAI;

class ResourceManagerMock : public ResourceManager
{	
	friend class ResourceManagerTest; //everything is public
public:
	using ResourceManager::ResourceManager;
	//access protected members, TODO: consider other architecture?
	void reserveResoures(TResources &res, Goals::TSubgoal goal = Goals::TSubgoal()) override;
	bool updateGoal(Goals::TSubgoal goal) override;
	bool notifyGoalCompleted(Goals::TSubgoal goal) override;
};