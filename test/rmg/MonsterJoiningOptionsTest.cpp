/*
 * MonsterJoiningOptionsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/rmg/MonsterJoiningOptions.h"

namespace test
{
using namespace ::testing;

class MonsterJoiningOptionsTest : public Test
{
};

TEST_F(MonsterJoiningOptionsTest, ApplyToEmptyOptionsUsesDefaults)
{
	/*
	 * Asserts: applyTo on a default-constructed options object writes the generator
	 * defaults onto the creature.
	 * Pass: HOSTILE, joiningPercentage -1, joinOnlyForMoney false.
	 * Fail: COMPLIANT (the CGCreature default) or any joining field other than the defaults.
	 */
	CGCreature guard(nullptr);
	rmg::MonsterJoiningOptions options;
	options.applyTo(guard);

	EXPECT_EQ(guard.initialCharacter, CGCreature::Character::HOSTILE);
	EXPECT_EQ(guard.joiningPercentage, static_cast<int8_t>(-1));
	EXPECT_FALSE(guard.joinOnlyForMoney);
}

TEST_F(MonsterJoiningOptionsTest, ApplyToAllFieldsIncludingCustomAggression)
{
	/*
	 * Asserts: applyTo writes every set field, including CUSTOM + aggression.
	 * Pass: CUSTOM, agression 7, joiningPercentage 50, joinOnlyForMoney true.
	 * Fail: HOSTILE leftover, aggression not copied, or joining fields ignored.
	 */
	CGCreature guard(nullptr);
	rmg::MonsterJoiningOptions options;
	options.character = CGCreature::Character::CUSTOM;
	options.aggression = 7;
	options.joiningPercentage = 50;
	options.joinOnlyForMoney = true;
	options.applyTo(guard);

	EXPECT_EQ(guard.initialCharacter, CGCreature::Character::CUSTOM);
	EXPECT_EQ(guard.agression, static_cast<int8_t>(7));
	EXPECT_EQ(guard.joiningPercentage, static_cast<int8_t>(50));
	EXPECT_TRUE(guard.joinOnlyForMoney);
}

TEST_F(MonsterJoiningOptionsTest, MergeTakesOverridePerFieldAndKeepsBase)
{
	/*
	 * Asserts: merge copies a set overlay field and keeps a base field the overlay
	 * leaves unset.
	 * Pass: character and aggression from overlay, joiningPercentage and joinOnlyForMoney from base.
	 * Fail: unset overlay fields wipe the base, or set overlay fields are ignored.
	 */
	rmg::MonsterJoiningOptions base;
	base.character = CGCreature::Character::HOSTILE;
	base.joiningPercentage = 50;
	base.joinOnlyForMoney = true;

	rmg::MonsterJoiningOptions overlay;
	overlay.character = CGCreature::Character::CUSTOM;
	overlay.aggression = 7;

	auto merged = rmg::MonsterJoiningOptions::merge(base, overlay);

	EXPECT_EQ(merged.character, CGCreature::Character::CUSTOM);
	EXPECT_EQ(merged.aggression, static_cast<int8_t>(7));
	EXPECT_EQ(merged.joiningPercentage, static_cast<int8_t>(50));
	EXPECT_EQ(merged.joinOnlyForMoney, true);
}

}
