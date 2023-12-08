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
#include "../AI/VCAI/Goals/Goals.h"
#include "mock_VCAI_CGoal.h"
#include "mock_VCAI.h"
#include "mock_ResourceManager.h"
#include "../mock/mock_CPSICallback.h"
#include "../lib/CGameInfoCallback.h"

using namespace Goals;
using namespace ::testing;

struct ResourceManagerTest : public Test//, public IResourceManager
{
	std::unique_ptr<ResourceManagerMock> rm;

	NiceMock<CPSICallbackMock> gcm;
	NiceMock<VCAIMock> aim;
	TSubgoal invalidGoal, gatherArmy, buildThis, buildAny, recruitHero;

	ResourceManagerTest()
	{
		rm = make_unique<NiceMock<ResourceManagerMock>>(&gcm, &aim);

		//note: construct new goal for modfications
		invalidGoal = sptr(StrictMock<InvalidGoalMock>());
		gatherArmy = sptr(StrictMock<GatherArmyGoalMock>());
		buildThis = sptr(StrictMock<BuildThis>());
		buildAny = sptr(StrictMock<Build>());
		recruitHero = sptr(StrictMock<RecruitHero>());

		//auto AI = CDynLibHandler::getNewAI("VCAI.dll");
		//SET_GLOBAL_STATE(AI);

		//gtest couldn't deduce default return value;
		ON_CALL(gcm, getTownsInfo(_))
			.WillByDefault(Return(std::vector < const CGTownInstance *>()));

		ON_CALL(gcm, getTownsInfo())
			.WillByDefault(Return(std::vector < const CGTownInstance *>()));

		ON_CALL(aim, getFlaggedObjects())
			.WillByDefault(Return(std::vector < const CGObjectInstance *>()));

		//enable if get unexpected exceptions
		//ON_CALL(gcm, getResourceAmount())
		//	.WillByDefault(Return(TResources()));
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

	rm->reserveResoures(armyCost, gatherArmy);
	EXPECT_FALSE(rm->canAfford(buildingCost)) << "Reserved value should be substracted from free resources";
}

TEST_F(ResourceManagerTest, notifyGoalImplemented)
{
	ASSERT_FALSE(rm->hasTasksLeft());

	EXPECT_FALSE(rm->notifyGoalCompleted(invalidGoal));
	EXPECT_FALSE(rm->hasTasksLeft());

	TResources res(0,0,0,0,0,0,12345);;
	rm->reserveResoures(res, invalidGoal);
	ASSERT_FALSE(rm->hasTasksLeft()) << "Can't push Invalid goal";
	EXPECT_FALSE(rm->notifyGoalCompleted(invalidGoal));

	EXPECT_FALSE(rm->notifyGoalCompleted(gatherArmy)) << "Queue should be empty";
	rm->reserveResoures(res, gatherArmy);
	EXPECT_TRUE(rm->notifyGoalCompleted(gatherArmy)) << "Not implemented"; //TODO: try it with not a copy
	EXPECT_FALSE(rm->notifyGoalCompleted(gatherArmy)); //already completed
}

TEST_F(ResourceManagerTest, notifyFulfillsAll)
{
	TResources res;
	ASSERT_TRUE(buildAny->fulfillsMe(buildThis)) << "Goal dependency implemented incorrectly"; //TODO: goal mock?
	rm->reserveResoures(res, buildAny);
	rm->reserveResoures(res, buildAny);
	rm->reserveResoures(res, buildAny);
	ASSERT_TRUE(rm->hasTasksLeft()); //regardless if duplicates are allowed or not
	rm->notifyGoalCompleted(buildThis);
	ASSERT_FALSE(rm->hasTasksLeft()) << "BuildThis didn't remove Build Any!";
}

TEST_F(ResourceManagerTest, queueOrder)
{
	ASSERT_FALSE(rm->hasTasksLeft());

	TSubgoal buildLow = sptr(StrictMock<BuildThis>()),
		buildBit = sptr(StrictMock<BuildThis>()),
		buildMed = sptr(StrictMock<BuildThis>()),
		buildHigh = sptr(StrictMock<BuildThis>()),
		buildVeryHigh = sptr(StrictMock<BuildThis>()),
		buildExtra = sptr(StrictMock<BuildThis>()),
		buildNotSoExtra = sptr(StrictMock<BuildThis>());

	buildLow->setpriority(0).setbid(1);
	buildLow->setpriority(1).setbid(2);
	buildMed->setpriority(2).setbid(3);
	buildHigh->setpriority(5).setbid(4);
	buildVeryHigh->setpriority(10).setbid(5);

	TResources price(0, 0, 0, 0, 0, 0, 1000);
	rm->reserveResoures(price, buildLow);
	rm->reserveResoures(price, buildHigh);
	rm->reserveResoures(price, buildMed);

	ON_CALL(gcm, getResourceAmount())
		.WillByDefault(Return(TResources(0,0,0,0,0,0,4000,0))); //we can afford 4 top goals

	auto goal = rm->whatToDo();
	EXPECT_EQ(goal->goalType, Goals::BUILD_STRUCTURE);
	ASSERT_EQ(rm->whatToDo()->bid, 4);
	rm->reserveResoures(price, buildBit);
	rm->reserveResoures(price, buildVeryHigh);
	goal = rm->whatToDo();
	EXPECT_EQ(goal->goalType, Goals::BUILD_STRUCTURE);
	ASSERT_EQ(goal->bid, 5);

	buildExtra->setpriority(3).setbid(100);
	EXPECT_EQ(rm->whatToDo(price, buildExtra)->bid, 100);

	buildNotSoExtra->setpriority(0.7f).setbid(7);
	goal = rm->whatToDo(price, buildNotSoExtra);
	EXPECT_NE(goal->bid, 7);
	EXPECT_EQ(goal->goalType, Goals::COLLECT_RES) << "We can't afford this goal, need to collect resources";
	EXPECT_EQ(goal->resID, EGameResID::GOLD) << "We need to collect gold";

	goal = rm->whatToDo();
	EXPECT_NE(goal->goalType, Goals::COLLECT_RES);
	EXPECT_EQ(goal->bid, 5) << "Should return highest-priority goal";
}

TEST_F(ResourceManagerTest, updateGoalImplemented)
{
	ASSERT_FALSE(rm->hasTasksLeft());

	TResources res;
	res[EGameResID::GOLD] = 12345;

	buildThis->setpriority(1);
	buildThis->bid = 666;

	EXPECT_FALSE(rm->updateGoal(buildThis)); //try update with no objectives -> fail

	rm->reserveResoures(res, buildThis);
	ASSERT_TRUE(rm->hasTasksLeft());
	buildThis->setpriority(4.444f);

	auto buildThis2 = sptr(StrictMock<BuildThis>());
	buildThis2->bid = 777;
	buildThis2->setpriority(3.33f);

	EXPECT_FALSE(rm->updateGoal(buildThis2)) << "Shouldn't update with wrong goal";
	EXPECT_TRUE(rm->updateGoal(buildThis)) << "Not implemented"; //try update with copy of reserved goal -> true

	EXPECT_FALSE(rm->updateGoal(invalidGoal)) << "Can't update Invalid goal";
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

TEST_F(ResourceManagerTest, reservedResources)
{
	//TOOO, not worth it now
}

TEST_F(ResourceManagerTest, freeResources)
{
	ON_CALL(gcm, getResourceAmount()) //in case callback or gs gets crazy
		.WillByDefault(Return(TResources(-1, 0, -13, -38763, -93764, -464, -12, -98765)));

	auto res = rm->freeResources();
	ASSERT_GE(res[EGameResID::WOOD], 0);
	ASSERT_GE(res[EGameResID::MERCURY], 0);
	ASSERT_GE(res[EGameResID::ORE], 0);
	ASSERT_GE(res[EGameResID::SULFUR], 0);
	ASSERT_GE(res[EGameResID::CRYSTAL], 0);
	ASSERT_GE(res[EGameResID::GEMS], 0);
	ASSERT_GE(res[EGameResID::GOLD], 0);
	ASSERT_GE(res[EGameResID::MITHRIL], 0);
}

TEST_F(ResourceManagerTest, freeResourcesWithManyGoals)
{
	ON_CALL(gcm, getResourceAmount())
		.WillByDefault(Return(TResources(20, 10, 20, 10, 10, 10, 20000, 0)));

	ASSERT_EQ(rm->freeResources(), TResources(20, 10, 20, 10, 10, 10, 20000, 0));

	rm->reserveResoures(TResources(0, 4, 0, 0, 0, 0, 13000), gatherArmy);
	ASSERT_EQ(rm->freeResources(), TResources(20, 6, 20, 10, 10, 10, 7000, 0));
	rm->reserveResoures(TResources(5, 4, 5, 4, 4, 4, 5000), buildThis);
	ASSERT_EQ(rm->freeResources(), TResources(15, 2, 15, 6, 6, 6, 2000, 0));
	rm->reserveResoures(TResources(0, 0, 0, 0, 0, 0, 2500), recruitHero);
	auto res = rm->freeResources();
	EXPECT_EQ(res[EGameResID::GOLD], 0) << "We should have 0 gold left";

	ON_CALL(gcm, getResourceAmount())
		.WillByDefault(Return(TResources(20, 10, 20, 10, 10, 10, 30000, 0))); //+10000 gold

	res = rm->freeResources();
	EXPECT_EQ(res[EGameResID::GOLD], 9500) << "We should have extra savings now";
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
