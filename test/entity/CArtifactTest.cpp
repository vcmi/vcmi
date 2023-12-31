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
        subject-> setImage(4242, "Test1", "Test2");

	auto cb = [this](auto && PH1, auto && PH2, auto && PH3, auto && PH4) 
	{
		registarCb(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4));
	};

	EXPECT_CALL(*this, registarCb(Eq(4242), Eq(0), "ARTIFACT", "Test1"));
	EXPECT_CALL(*this, registarCb(Eq(4242), Eq(0), "ARTIFACTLARGE", "Test2"));

	subject->registerIcons(cb);
}

}
