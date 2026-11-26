/*
 * TownFactionFallbackTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/rmg/Functions.h"

namespace test
{

using namespace ::testing;

class TownFactionFallbackTest : public Test
{
};

TEST_F(TownFactionFallbackTest, CastleHasTown)
{
	/*
	 * Asserts: a faction with a town object is accepted.
	 * Pass: CASTLE is true.
	 * Fail: CASTLE is treated as town-less.
	 */
	EXPECT_TRUE(factionHasTown(FactionID::CASTLE));
}

TEST_F(TownFactionFallbackTest, NeutralHasNoTown)
{
	/*
	 * Asserts: NEUTRAL is not a faction with a town object.
	 * Pass: false.
	 * Fail: NEUTRAL is treated as a town faction.
	 */
	EXPECT_FALSE(factionHasTown(FactionID::NEUTRAL));
}

TEST_F(TownFactionFallbackTest, RandomHasNoTown)
{
	/*
	 * Asserts: RANDOM is not a resolved faction.
	 * Pass: false.
	 * Fail: RANDOM is treated as a town faction.
	 */
	EXPECT_FALSE(factionHasTown(FactionID::RANDOM));
}

TEST_F(TownFactionFallbackTest, NoneHasNoTown)
{
	/*
	 * Asserts: NONE is not a faction with a town object.
	 * Pass: false.
	 * Fail: NONE is treated as a town faction.
	 */
	EXPECT_FALSE(factionHasTown(FactionID::NONE));
}

TEST_F(TownFactionFallbackTest, HintedTownFactionIsKeptWhenAllowedOmitsIt)
{
	/*
	 * Asserts: a hinted faction that has a town object is returned unchanged, even when
	 * it is not in allowed.
	 * Pass: CASTLE is returned when allowed is {RAMPART}.
	 * Fail: the result is drawn from allowed, or CASTLE is rejected.
	 */
	CRandomGenerator rng(1);
	const std::set<FactionID> allowed{FactionID::RAMPART};
	EXPECT_EQ(townFactionOrRandom(FactionID::CASTLE, allowed, rng), FactionID::CASTLE);
}

TEST_F(TownFactionFallbackTest, NeutralHintPicksFromAllowedTownFactions)
{
	/*
	 * Asserts: a hinted faction without a town object is replaced by a random faction
	 * from allowed that has one.
	 * Pass: over many seeds the result is always RAMPART or TOWER when those are allowed.
	 * Fail: the hinted NEUTRAL is kept, or a faction outside allowed is returned.
	 */
	const std::set<FactionID> allowed{FactionID::RAMPART, FactionID::TOWER};
	for(int seed = 1; seed <= 64; ++seed)
	{
		CRandomGenerator rng(seed);
		const FactionID result = townFactionOrRandom(FactionID::NEUTRAL, allowed, rng);
		EXPECT_TRUE(result == FactionID::RAMPART || result == FactionID::TOWER) << "seed=" << seed;
	}
}

TEST_F(TownFactionFallbackTest, NeutralInAllowedIsSkipped)
{
	/*
	 * Asserts: allowed is filtered to factions with a town object before the random pick.
	 * Pass: NEUTRAL hinted with allowed {NEUTRAL, TOWER} is always TOWER.
	 * Fail: NEUTRAL is returned because it is in allowed.
	 */
	const std::set<FactionID> allowed{FactionID::NEUTRAL, FactionID::TOWER};
	for(int seed = 1; seed <= 16; ++seed)
	{
		CRandomGenerator rng(seed);
		EXPECT_EQ(townFactionOrRandom(FactionID::NEUTRAL, allowed, rng), FactionID::TOWER) << "seed=" << seed;
	}
}

TEST_F(TownFactionFallbackTest, EmptyAllowedPicksATownFaction)
{
	/*
	 * Asserts: when allowed has no faction with a town object, the result is taken from
	 * the library's default town factions.
	 * Pass: the result has a town object.
	 * Fail: NEUTRAL or another faction without a town is returned.
	 */
	CRandomGenerator rng(1);
	const FactionID result = townFactionOrRandom(FactionID::NEUTRAL, {}, rng);
	EXPECT_TRUE(factionHasTown(result));
}

TEST_F(TownFactionFallbackTest, InvalidHintPicksFromAllowed)
{
	/*
	 * Asserts: a faction that is not valid counts as having no town, so the result is
	 * drawn from allowed.
	 * Pass: NONE hinted with allowed {RAMPART, TOWER} returns RAMPART or TOWER.
	 * Fail: NONE is returned.
	 */
	const std::set<FactionID> allowed{FactionID::RAMPART, FactionID::TOWER};
	CRandomGenerator rng(1);
	const FactionID result = townFactionOrRandom(FactionID::NONE, allowed, rng);
	EXPECT_TRUE(result == FactionID::RAMPART || result == FactionID::TOWER);
}

}
