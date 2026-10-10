/*
 * StackExperienceBonusesTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/CCreatureHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/bonuses/Limiters.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"

/// Stack experience gives a stack of each rank a single bonus with the total value of that rank, so that
/// the value can be compared against other bonuses, for example through the `stacking` field.
TEST(StackExperienceBonusesTest, OneBonusWithTotalValuePerRank)
{
	const auto identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), std::string("creature"), std::string("vcmi-test:testStackExperience"));
	ASSERT_TRUE(identifier.has_value());
	const CCreature * creature = CreatureID(*identifier).toCreature();

	// values of the config for ranks 1 to 10; rank 0 has no bonus and rank 11 keeps the value of rank 10
	const std::vector<int> expected = { 0, 0, 1, 1, 2, 2, 2, 3, 3, 1, 5, 5 };

	for(int rank = 0; rank < static_cast<int>(expected.size()); ++rank)
	{
		int bonusCount = 0;
		int value = 0;
		for(const auto & bonus : creature->getExportedBonusList())
		{
			if(bonus->source != BonusSource::STACK_EXPERIENCE)
				continue;

			const auto limiter = std::dynamic_pointer_cast<const RankRangeLimiter>(bonus->limiter);
			ASSERT_NE(limiter, nullptr);

			if(rank > limiter->minRank && rank < limiter->maxRank)
			{
				++bonusCount;
				value += bonus->val;
			}
		}

		EXPECT_LE(bonusCount, 1) << "rank " << rank;
		EXPECT_EQ(value, expected[rank]) << "rank " << rank;
	}
}
