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

#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

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

using Nullkiller2_Analyzers_ArmyManagerMap = NullkillerTest;

TEST_F(Nullkiller2_Analyzers_ArmyManagerMap, targetKeepsOwnArmyWhenOnlyWantedUnitMustStayInSource)
{
	const PlayerColor player(0);

	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero({5, 5, 0}, HeroTypeID(0), player)
		.heroGarrison({{CreatureID::SKELETON, 1}})
		.hero({6, 5, 0}, HeroTypeID(1), player)
		.heroGarrison({{CreatureID(13), 1}});
	startWithMap(std::move(builder));

	const auto gateway = makeGateway(player);
	const auto * target = findHeroAt({5, 5, 0});
	const auto * source = findHeroAt({6, 5, 0});
	ASSERT_NE(target, nullptr);
	ASSERT_NE(source, nullptr);

	// undead would only lower morale of the archangel (13), which is the last unit of its hero
	const auto army = gateway->nullkiller->armyManager->getBestArmy(target, target, source, TerrainId::GRASS);

	ASSERT_EQ(army.size(), 1) << "an empty army makes the AI move away the last unit of the target";
	EXPECT_EQ(army.front().creature->getId(), CreatureID::SKELETON);
}
