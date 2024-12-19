/*
 * mock_ResourceManager.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock_ResourceManager.h"

void ResourceManagerMock::reserveResources(const TResources &res, Goals::TSubgoal goal)
{
	ResourceManager::reserveResources(res, goal);
}

bool ResourceManagerMock::updateGoal(Goals::TSubgoal goal)
{
	return ResourceManager::updateGoal(goal);
}

bool ResourceManagerMock::notifyGoalCompleted(Goals::TSubgoal goal)
{
	return ResourceManager::notifyGoalCompleted(goal);
}
