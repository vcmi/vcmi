/*
 * CHeroClassTest.cpp, part of VCMI engine
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

class CHeroClassTest : public Test
{
public:
	MOCK_METHOD4(registarCb, void(int32_t, int32_t, const std::string &, const std::string &));

protected:
	std::shared_ptr<CHeroClass> subject;

	void SetUp() override
	{
		subject = std::make_shared<CHeroClass>();
	}
};

TEST_F(CHeroClassTest, RegistersIconsDoesNothing)
{
	auto cb = std::bind(&CHeroClassTest::registarCb, this, _1, _2, _3, _4);
	subject->registerIcons(cb);
}

}
