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
	//Do not modify to ensure correct testing
	MOCK_METHOD2(reserveResoures, void(TResources &res, Goals::TSubgoal goal));
	MOCK_METHOD1(updateGoal, bool(Goals::TSubgoal goal));
	MOCK_METHOD1(notifyGoalCompleted, bool(Goals::TSubgoal goal));
};