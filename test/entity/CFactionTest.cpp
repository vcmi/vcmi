/*
 * CFactionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/CTownHandler.h"

namespace test
{

using namespace ::testing;

class CFactionTest : public Test
{
public:
	MOCK_METHOD4(registarCb, void(int32_t, int32_t, const std::string &, const std::string &));

protected:
	std::shared_ptr<CFaction> subject;

	void SetUp() override
	{
		subject = std::make_shared<CFaction>();
	}
};

TEST_F(CFactionTest, HasTown)
{
	EXPECT_FALSE(subject->hasTown());
	subject->town = new CTown();
	EXPECT_TRUE(subject->hasTown());
	vstd::clear_pointer(subject->town);
	EXPECT_FALSE(subject->hasTown());
}

TEST_F(CFactionTest, RegistersNoIconsIfNoTown)
{
	auto cb = [this](auto && PH1, auto && PH2, auto && PH3, auto && PH4) 
	{
		registarCb(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4));
	};
	subject->registerIcons(cb);
}

TEST_F(CFactionTest, RegistersIcons)
{
	auto cb = [this](auto && PH1, auto && PH2, auto && PH3, auto && PH4) 
	{
		registarCb(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4));
	};

	subject->town = new CTown();

	CTown::ClientInfo & info = subject->town->clientInfo;

	info.icons[0][0] = 10;
	info.icons[0][1] = 11;
	info.icons[1][0] = 12;
	info.icons[1][1] = 13;

	info.iconLarge[0][0] = "Test1";
	info.iconLarge[0][1] = "Test2";
	info.iconLarge[1][0] = "Test3";
	info.iconLarge[1][1] = "Test4";

	info.iconSmall[0][0] = "Test10";
	info.iconSmall[0][1] = "Test20";
	info.iconSmall[1][0] = "Test30";
	info.iconSmall[1][1] = "Test40";
	info.towerIconSmall = "Test5";
	info.towerIconLarge = "Test6";


	EXPECT_CALL(*this, registarCb(Eq(10), Eq(0), "ITPT", "Test1"));
	EXPECT_CALL(*this, registarCb(Eq(11), Eq(0), "ITPT", "Test2"));
	EXPECT_CALL(*this, registarCb(Eq(12), Eq(0), "ITPT", "Test3"));
	EXPECT_CALL(*this, registarCb(Eq(13), Eq(0), "ITPT", "Test4"));

	EXPECT_CALL(*this, registarCb(Eq(12), Eq(0), "ITPA", "Test10"));
	EXPECT_CALL(*this, registarCb(Eq(13), Eq(0), "ITPA", "Test20"));
	EXPECT_CALL(*this, registarCb(Eq(14), Eq(0), "ITPA", "Test30"));
	EXPECT_CALL(*this, registarCb(Eq(15), Eq(0), "ITPA", "Test40"));

	EXPECT_CALL(*this, registarCb(Eq(subject->getIconIndex()), Eq(1), "CPRSMALL", "Test5"));
	EXPECT_CALL(*this, registarCb(Eq(subject->getIconIndex()), Eq(1), "TWCRPORT", "Test6"));

	subject->registerIcons(cb);
}

}
