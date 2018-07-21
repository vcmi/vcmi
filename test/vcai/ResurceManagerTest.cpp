/*
* ResourceManagerTest.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "gtest/gtest.h"

#include "../AI/VCAI/ResourceManager.h"
#include "../AI/VCAI/Goals.h"
#include "mock_VCAI_CGoal.h"
#include "../mock/mock_CPlayerSpecificInfoCallback.h"

using namespace Goals;
using namespace ::testing;

struct ResourceManagerTest : public Test
{
	std::unique_ptr<ResourceManager> rm;

	StrictMock<InvalidGoalMock> igm;
	StrictMock<GatherArmyGoalMock> gam;
	StrictMock<GameCallbackMock> cb;
	ResourceManagerTest()
	{
		rm = std::make_unique<ResourceManager>();

		//auto AI = CDynLibHandler::getNewAI("VCAI.dll");
		//SET_GLOBAL_STATE(AI);
	}
};

TEST_F(ResourceManagerTest, canAffordMaths)
{
	//mocking cb calls inside canAfford()

	ON_CALL(cb, getResourceAmount())
		.WillByDefault(InvokeWithoutArgs([]() -> TResources
		{
			return TResources(10, 0, 11, 0, 0, 0, 12345);
		}));

	TResources buildingCost(10, 0, 10, 0, 0, 0, 5000);
	EXPECT_TRUE(rm->canAfford(buildingCost));

	TResources armyCost(0, 0, 0, 0, 0, 0, 54321);
	EXPECT_FALSE(rm->canAfford(armyCost));
}

TEST_F(ResourceManagerTest, notifyGoalImplemented)
{
	ASSERT_FALSE(rm->hasTasksLeft());

	auto ig = sptr(igm);
	EXPECT_FALSE(rm->notifyGoalCompleted(ig));
	EXPECT_FALSE(rm->hasTasksLeft());

	TResources res;
	res[Res::GOLD] = 12345;
	rm->reserveResoures(res, ig);
	ASSERT_TRUE(rm->hasTasksLeft()); //TODO: invalid shouldn't be accepted or pushed
	EXPECT_FALSE(rm->notifyGoalCompleted(ig)); //TODO: invalid should never be completed

	auto ga = sptr(gam);
	EXPECT_FALSE(rm->notifyGoalCompleted(ga));
	rm->reserveResoures(res, ga);
	EXPECT_TRUE(rm->notifyGoalCompleted(ga)); //TODO: try it with not a copy
	EXPECT_FALSE(rm->notifyGoalCompleted(ga)); //already completed
}

TEST_F(ResourceManagerTest, complexGoalNotify)
{
	ASSERT_FALSE(rm->hasTasksLeft());
	//TODO
}

TEST_F(ResourceManagerTest, updateGoalImplemented)
{
	ASSERT_FALSE(rm->hasTasksLeft());

	TResources res;
	res[Res::GOLD] = 12345;

	auto ga = sptr(gam);
	ga->setpriority(1);
	ga->objid = 666; //FIXME: which property actually can be used to tell GatherArmy apart?

	EXPECT_FALSE(rm->updateGoal(ga)); //try update with no objectives -> fail

	rm->reserveResoures(res, ga);
	ASSERT_TRUE(rm->hasTasksLeft());
	ga->setpriority(4.444f);

	auto ga2 = sptr(StrictMock<GatherArmyGoalMock>());
	ga->objid = 777; //FIXME: which property actually can be used to tell GatherArmy apart?
	ga2->setpriority(3.33f);

	EXPECT_FALSE(rm->updateGoal(ga2)); //try update with wrong goal -> fail
	EXPECT_TRUE(rm->updateGoal(ga)); //try update with copy of reserved goal -> true

	auto ig = sptr(igm);
	EXPECT_FALSE(rm->updateGoal(ig)); //invalid goal must fail
}

TEST_F(ResourceManagerTest, complexGoalUpdates)
{
	ASSERT_FALSE(rm->hasTasksLeft());
	//TODO
}