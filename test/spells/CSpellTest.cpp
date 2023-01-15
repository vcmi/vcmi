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
	MOCK_METHOD4(registarCb, void(int32_t, int32_t, const std::string &, const std::string &));
	std::shared_ptr<CSpell> subject;
protected:
	void SetUp() override
	{
		subject = std::make_shared<CSpell>();
		subject->iconBook = "Test1";
		subject->iconEffect = "Test2";
		subject->iconScenarioBonus = "Test3";
		subject->iconScroll = "Test4";
	}
};

TEST_F(CSpellTest, RegistersIcons)
{
	subject->id = SpellID(42);

	auto cb = std::bind(&CSpellTest::registarCb, this, _1, _2, _3, _4);

	EXPECT_CALL(*this, registarCb(Eq(42), Eq(0), "SPELLS", "Test1"));
	EXPECT_CALL(*this, registarCb(Eq(43), Eq(0), "SPELLINT", "Test2"));
	EXPECT_CALL(*this, registarCb(Eq(42), Eq(0), "SPELLBON", "Test3"));
	EXPECT_CALL(*this, registarCb(Eq(42), Eq(0), "SPELLSCR", "Test4"));

	subject->registerIcons(cb);
}

}
