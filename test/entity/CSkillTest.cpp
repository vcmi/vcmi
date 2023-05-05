/*
 * CSkillTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/CSkillHandler.h"

namespace test
{

using namespace ::testing;

class CSkillTest : public Test
{
public:
	MOCK_METHOD4(registarCb, void(int32_t, int32_t, const std::string &, const std::string &));

protected:
	std::shared_ptr<CSkill> subject;

	void SetUp() override
	{
		subject = std::make_shared<CSkill>(SecondarySkill(42));
	}
};

TEST_F(CSkillTest, RegistersIcons)
{
	for(int level = 1; level <= 3; level++)
	{
		CSkill::LevelInfo & skillAtLevel = subject->at(level);

		skillAtLevel.iconSmall = "TestS"+std::to_string(level);
		skillAtLevel.iconMedium = "TestM"+std::to_string(level);
		skillAtLevel.iconLarge = "TestL"+std::to_string(level);
	}

	auto cb = [this](auto && PH1, auto && PH2, auto && PH3, auto && PH4) 
	{
		registarCb(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4));
	};

	for(int level = 1; level <= 3; level++)
	{
		int frame = 2 + level + 3 * 42;

		EXPECT_CALL(*this, registarCb(Eq(frame), Eq(0), "SECSK32", "TestS"+std::to_string(level)));
		EXPECT_CALL(*this, registarCb(Eq(frame), Eq(0), "SECSKILL", "TestM"+std::to_string(level)));
		EXPECT_CALL(*this, registarCb(Eq(frame), Eq(0), "SECSK82", "TestL"+std::to_string(level)));
	}

	subject->registerIcons(cb);
}

}
