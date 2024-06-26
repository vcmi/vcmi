/*
 * mock_ResourceManager.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "gtest/gtest.h"
#include "../AI/VCAI/ResourceManager.h"

class ResourceManager;
class CPlayerSpecificInfoCallback;
class VCAI;

class ResourceManagerMock : public ResourceManager
{	
	friend struct ResourceManagerTest; //everything is public
public:
	using ResourceManager::ResourceManager;
	//access protected members, TODO: consider other architecture?
	void reserveResources(const TResources &res, Goals::TSubgoal goal = Goals::TSubgoal()) override;
	bool updateGoal(Goals::TSubgoal goal) override;
	bool notifyGoalCompleted(Goals::TSubgoal goal) override;
};