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

#include "../AI/VCAI/VCAI.h"
#include "ResourceManagerTest.h"
#include "../AI/VCAI/ResourceManager.h"
#include "../AI/VCAI/Goals.h"
#include "mock_VCAI_CGoal.h"
#include "../mock/mock_CPSICallback.h"
#include "../lib/CGameInfoCallback.h"

using namespace Goals;
using namespace ::testing;

struct ResourceManagerTest : public Test
{
	std::unique_ptr<ResourceManager> rm;

	StrictMock<InvalidGoalMock> igm;
	StrictMock<GatherArmyGoalMock> gam;
	NiceMock<GameCallbackMock> gcm;

	ResourceManagerTest()
	{
		rm = std::make_unique<ResourceManager>(&gcm);

		//auto AI = CDynLibHandler::getNewAI("VCAI.dll");
		//SET_GLOBAL_STATE(AI);

		ON_CALL(gcm, getTownsInfo(_)) //TODO: probably call to this function needs to be dropped altogether
			.WillByDefault(Return(std::vector < const CGTownInstance *>())); //gtest couldn't deduce default return value);
	}
};

TEST_F(ResourceManagerTest, canAffordMaths)
{
	//mocking cb calls inside canAfford()

	ON_CALL(gcm, getResourceAmount())
		.WillByDefault(Return(TResources(10, 0, 11, 0, 0, 0, 12345)));

	TResources buildingCost(10, 0, 10, 0, 0, 0, 5000);
	//EXPECT_CALL(gcm, getResourceAmount()).RetiresOnSaturation();
	//EXPECT_CALL(gcm, getTownsInfo(_)).RetiresOnSaturation();
	EXPECT_NO_THROW(rm->canAfford(buildingCost));
	EXPECT_TRUE(rm->canAfford(buildingCost));

	TResources armyCost(0, 0, 0, 0, 0, 0, 54321);
	EXPECT_FALSE(rm->canAfford(armyCost));

	auto ga = sptr(gam);
	rm->reserveResoures(armyCost, ga);
	EXPECT_FALSE(rm->canAfford(buildingCost)) << "Reserved value should be substracted from free resources";
}

TEST_F(ResourceManagerTest, notifyGoalImplemented)
{
	ASSERT_FALSE(rm->hasTasksLeft());

	auto ig = sptr(igm);
	EXPECT_FALSE(rm->notifyGoalCompleted(ig));
	EXPECT_FALSE(rm->hasTasksLeft());

	TResources res(0,0,0,0,0,0,12345);;
	rm->reserveResoures(res, ig);
	ASSERT_FALSE(rm->hasTasksLeft()) << "Can't push Invalid goal";
	EXPECT_FALSE(rm->notifyGoalCompleted(ig));

	auto ga = sptr(gam);
	EXPECT_FALSE(rm->notifyGoalCompleted(ga)) << "Queue should be empty";
	rm->reserveResoures(res, ga);
	EXPECT_TRUE(rm->notifyGoalCompleted(ga)) << "Not implemented"; //TODO: try it with not a copy
	EXPECT_FALSE(rm->notifyGoalCompleted(ga)); //already completed
}

TEST_F(ResourceManagerTest, complexGoalNotify)
{
	//TODO
	ASSERT_FALSE(rm->hasTasksLeft());
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
	EXPECT_TRUE(rm->updateGoal(ga)) << "Not implemented"; //try update with copy of reserved goal -> true

	auto ig = sptr(igm);
	EXPECT_FALSE(rm->updateGoal(ig)); //invalid goal must fail
}

TEST_F(ResourceManagerTest, complexGoalUpdates)
{
	//TODO
	ASSERT_FALSE(rm->hasTasksLeft());
}

TEST_F(ResourceManagerTest, tasksLeft)
{
	ASSERT_FALSE(rm->hasTasksLeft());
}

TEST_F(ResourceManagerTest, freeGold)
{
	ON_CALL(gcm, getResourceAmount())
		.WillByDefault(Return(TResources(0, 0, 0, 0, 0, 0, 666)));

	ASSERT_EQ(rm->freeGold(), 666);

	ON_CALL(gcm, getResourceAmount())
		.WillByDefault(Return(TResources(0, 0, 0, 0, 0, 0, -3762363)));

	ASSERT_GE(rm->freeGold(), 0) << "We should never see negative savings";
}