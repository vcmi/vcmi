/*
 * BuyArmyTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Goals/BuyArmy.h"

#include "lib/mapObjects/CGHeroInstance.h"

namespace
{
void fillFullArmy(CCreatureSet & army)
{
	const std::array<CreatureID, GameConstants::ARMY_SIZE> creatures = {
		CreatureID(CreatureID::ARCHER),
		CreatureID(0),
		CreatureID(1),
		CreatureID(4),
		CreatureID(5),
		CreatureID(6),
		CreatureID(7)
	};

	for(size_t index = 0; index < creatures.size(); ++index)
		ASSERT_TRUE(army.setCreature(SlotID(index), creatures[index], 1));
}
}

TEST(Nullkiller2_Goals_BuyArmy, fullArmyDoesNotNeedFreeSlotForExistingCreature)
{
	CGHeroInstance army(nullptr);
	fillFullArmy(army);

	ASSERT_EQ(army.stacksCount(), GameConstants::ARMY_SIZE);

	EXPECT_FALSE(NK2AI::Goals::BuyArmy::needsFreeSlotToRecruit(
		&army,
		CreatureID(CreatureID::ARCHER)))
		<< "recruiting an existing creature type can merge without dismissing a stack";

	EXPECT_TRUE(NK2AI::Goals::BuyArmy::needsFreeSlotToRecruit(
		&army,
		CreatureID(8)))
		<< "a full army still needs a free slot for a missing creature type";
}

TEST(Nullkiller2_Goals_BuyArmy, partialArmyDoesNotNeedFreeSlotForMissingCreature)
{
	CGHeroInstance army(nullptr);
	ASSERT_TRUE(army.setCreature(SlotID(0), CreatureID(CreatureID::ARCHER), 1));

	EXPECT_FALSE(NK2AI::Goals::BuyArmy::needsFreeSlotToRecruit(
		&army,
		CreatureID(8)));
}
