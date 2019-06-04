/*
 * CSpellTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/spells/CSpellHandler.h"

namespace test
{
using namespace ::spells;
using namespace ::testing;

class CSpellTest : public Test
{
public:
	MOCK_METHOD3(registarCb, void(int32_t, const std::string &, const std::string &));

protected:
	std::shared_ptr<CSpell> subject;

	void SetUp() override
	{
		subject = std::make_shared<CSpell>();
		subject->id = SpellID(42);
		subject->iconBook = "Test1";
		subject->iconEffect = "Test2";
		subject->iconScenarioBonus = "Test3";
		subject->iconScroll = "Test4";
	}
};

TEST_F(CSpellTest, RegistersIcons)
{
	auto cb = std::bind(&CSpellTest::registarCb, this, _1, _2, _3);

	EXPECT_CALL(*this, registarCb(Eq(42), "SPELLS", "Test1"));
	EXPECT_CALL(*this, registarCb(Eq(43), "SPELLINT", "Test2"));
	EXPECT_CALL(*this, registarCb(Eq(42), "SPELLBON", "Test3"));
	EXPECT_CALL(*this, registarCb(Eq(42), "SPELLSCR", "Test4"));

	subject->registerIcons(cb);
}

}
