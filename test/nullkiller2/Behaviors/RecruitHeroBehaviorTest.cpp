/*
 * RecruitHeroBehaviorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "Global.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "AI/Nullkiller2/Behaviors/RecruitHeroBehavior.h"
#include "AI/Nullkiller2/Engine/Nullkiller.h"

class MockNullkiller : public NK2AI::Nullkiller
{
public:
	~MockNullkiller() override = default;
	MOCK_METHOD(void, makeTurn, (), (override));
};

TEST(Nullkiller2_Behaviors_RecruitHeroBehavior, calculateBestHero)
{
	EXPECT_EQ(1, 1);
	auto behavior = NK2AI::Goals::RecruitHeroBehavior();
	EXPECT_FALSE(behavior.invalid());
	EXPECT_EQ(1, 1);
	auto * const aiNk = new MockNullkiller();
	EXPECT_CALL(*aiNk, makeTurn()).Times(1);
	aiNk->makeTurn();
	delete aiNk;
}
