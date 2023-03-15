/*
 * CHeroTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/CHeroHandler.h"

namespace test
{

using namespace ::testing;

class CHeroTest : public Test
{
public:
	MOCK_METHOD4(registarCb, void(int32_t, int32_t, const std::string &, const std::string &));

protected:
	std::shared_ptr<CHero> subject;

	void SetUp() override
	{
		subject = std::make_shared<CHero>();
	}
};

TEST_F(CHeroTest, RegistersIcons)
{
	subject->imageIndex = 4242;

	subject->iconSpecSmall = "Test1";
	subject->iconSpecLarge = "Test2";
	subject->portraitSmall = "Test3";
	subject->portraitLarge = "Test4";

	auto cb = std::bind(&CHeroTest::registarCb, this, _1, _2, _3, _4);

	EXPECT_CALL(*this, registarCb(Eq(4242), Eq(0), "UN32", "Test1"));
	EXPECT_CALL(*this, registarCb(Eq(4242), Eq(0), "UN44", "Test2"));
	EXPECT_CALL(*this, registarCb(Eq(4242), Eq(0), "PORTRAITSSMALL", "Test3"));
	EXPECT_CALL(*this, registarCb(Eq(4242), Eq(0), "PORTRAITSLARGE", "Test4"));

	subject->registerIcons(cb);
}

}
