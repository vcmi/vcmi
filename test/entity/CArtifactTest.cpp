/*
 * CArtifactTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/CArtHandler.h"

namespace test
{

using namespace ::testing;

class CArtifactTest : public Test
{
public:
	MOCK_METHOD4(registarCb, void(int32_t, int32_t, const std::string &, const std::string &));

protected:
	std::shared_ptr<CArtifact> subject;

	void SetUp() override
	{
		subject = std::make_shared<CArtifact>();
	}
};

TEST_F(CArtifactTest, RegistersIcons)
{
	subject->iconIndex = 4242;
	subject->image = "Test1";
	subject->large = "Test2";

	auto cb = std::bind(&CArtifactTest::registarCb, this, _1, _2, _3, _4);

	EXPECT_CALL(*this, registarCb(Eq(4242), Eq(0), "ARTIFACT", "Test1"));
	EXPECT_CALL(*this, registarCb(Eq(4242), Eq(0), "ARTIFACTLARGE", "Test2"));

	subject->registerIcons(cb);
}

}
