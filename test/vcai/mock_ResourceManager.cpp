#include "StdInc.h"

#include "mock_ResourceManager.h"

void ResourceManagerMock::reserveResoures(const TResources &res, Goals::TSubgoal goal)
{
	ResourceManager::reserveResoures(res, goal);
}

bool ResourceManagerMock::updateGoal(Goals::TSubgoal goal)
{
	return ResourceManager::updateGoal(goal);
}

bool ResourceManagerMock::notifyGoalCompleted(Goals::TSubgoal goal)
{
	return ResourceManager::notifyGoalCompleted(goal);
}
