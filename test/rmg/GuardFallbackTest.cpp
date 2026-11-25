/*
 * GuardFallbackTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/rmg/Functions.h"

namespace test
{

using namespace ::testing;

class GuardFallbackTest : public Test
{
protected:
	const CreatureID troglodyte{CreatureID::TROGLODYTES};
	const CreatureID pikeman{0};
	const CreatureID blackDragon{83};

	const GuardCandidate troglodyteCandidate{troglodyte, 59, 2065, 5900};
	const GuardCandidate pikemanCandidate{pikeman, 80, 2800, 8000};
	const GuardCandidate blackDragonCandidate{blackDragon, 8721, 47965, 872100};
};

TEST_F(GuardFallbackTest, StrengthBelowWindowsPicksTroglodyteInAnyOrder)
{
	const std::vector<std::vector<GuardCandidate>> orders{
		{troglodyteCandidate, pikemanCandidate, blackDragonCandidate},
		{troglodyteCandidate, blackDragonCandidate, pikemanCandidate},
		{pikemanCandidate, troglodyteCandidate, blackDragonCandidate},
		{pikemanCandidate, blackDragonCandidate, troglodyteCandidate},
		{blackDragonCandidate, troglodyteCandidate, pikemanCandidate},
		{blackDragonCandidate, pikemanCandidate, troglodyteCandidate},
	};

	for(const auto & candidates : orders)
		EXPECT_EQ(chooseFallbackGuard(2000, candidates), troglodyte);
}

TEST_F(GuardFallbackTest, StrengthAboveWindowsPicksBlackDragon)
{
	const std::vector candidates{troglodyteCandidate, pikemanCandidate, blackDragonCandidate};
	EXPECT_EQ(chooseFallbackGuard(900000, candidates), blackDragon);
}

TEST_F(GuardFallbackTest, StrengthInGapPicksNearerCandidate)
{
	const CreatureID nearer{1};
	const CreatureID farther{2};
	const GuardCandidate low{nearer, 10, 1000, 2000};
	const GuardCandidate high{farther, 10, 8000, 9000};

	EXPECT_EQ(chooseFallbackGuard(3000, {low, high}), nearer);
	EXPECT_EQ(chooseFallbackGuard(7000, {low, high}), farther);
}

TEST_F(GuardFallbackTest, EmptyCandidatesYieldPeasant)
{
	EXPECT_EQ(chooseFallbackGuard(2000, {}), CreatureID::PEASANT);
}

TEST_F(GuardFallbackTest, TiePicksFirstCandidate)
{
	const CreatureID first{1};
	const CreatureID second{2};
	const GuardCandidate a{first, 10, 1000, 2000};
	const GuardCandidate b{second, 10, 4000, 5000};

	EXPECT_EQ(chooseFallbackGuard(3000, {a, b}), first);
	EXPECT_EQ(chooseFallbackGuard(3000, {b, a}), second);
}

TEST_F(GuardFallbackTest, GuardStackSizeNeverBelowOne)
{
	EXPECT_EQ(guardStackSize(2000, 78845), 1);
	EXPECT_EQ(guardStackSize(2000, 59), 33);
}

}
