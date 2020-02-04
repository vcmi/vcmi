/*
 * BattleInfoTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "lib/battle/obstacle/StaticObstacle.h"
#include "mock/mock_ObstacleJson.h"
#include "lib/battle/BattleInfo.h"

using namespace testing;

class BattleInfoTest : public ::testing::Test, public BattleInfo
{
public:
	std::shared_ptr<ObstacleJsonMock> jsonMock;
	BattleInfoTest()
	{
		jsonMock = std::make_shared<ObstacleJsonMock>();
	}
private:
};

TEST_F(BattleInfoTest, inherentObstacles)
{
}
