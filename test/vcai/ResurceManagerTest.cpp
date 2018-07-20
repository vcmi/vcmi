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

using namespace Goals;
using namespace ::testing;

struct ResourceManagerTest : public Test
{
	std::unique_ptr<ResourceManager> rm;

	StrictMock<InvalidGoalMock> igm;
	ResourceManagerTest()
	{
		rm = std::make_unique<ResourceManager>();
	}
};

TEST_F(ResourceManagerTest, notifyGoalImplemented)
{
	auto g = sptr(igm);
	EXPECT_EQ(rm->notifyGoalCompleted(g), false);
}

TEST_F(ResourceManagerTest, updateGoalImplemented)
{
	//TODO: 
	//try update with no objectives -> fail

	//reserve resources

	//try update with wrong goal -> fail
	//try update with copy of reserved goal -> true

	//try different goal subclasses comparisons

	auto g = sptr(igm); //invalid goal must fail
	EXPECT_EQ(rm->updateGoal(g), false);
}

//TEST(testMath, myCubeTest)
//{
//	EXPECT_EQ(1000, cubic(10));
//}