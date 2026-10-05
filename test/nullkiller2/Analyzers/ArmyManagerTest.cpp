/*
 * ArmyManagerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Analyzers/ArmyManager.h"

#include "lib/mapObjects/CGHeroInstance.h"

TEST(Nullkiller2_Analyzers_ArmyManager, armyWithoutOneSourceCreatureDoesNotTakeWholeSource)
{
	CGHeroInstance source(nullptr);
	ASSERT_TRUE(source.setCreature(SlotID(0), CreatureID(0), 1));
	ASSERT_TRUE(source.setCreature(SlotID(1), CreatureID(14), 5));

	// e.g. the best army kept only one faction, so creature 14 stays in the source
	const std::vector<NK2AI::SlotInfo> partial = { { CreatureID(0).toCreature(), 1, 1 } };
	EXPECT_FALSE(NK2AI::ArmyManager::takesWholeSource(&source, partial));

	const std::vector<NK2AI::SlotInfo> whole = { { CreatureID(0).toCreature(), 1, 1 }, { CreatureID(14).toCreature(), 5, 5 } };
	EXPECT_TRUE(NK2AI::ArmyManager::takesWholeSource(&source, whole));
}
