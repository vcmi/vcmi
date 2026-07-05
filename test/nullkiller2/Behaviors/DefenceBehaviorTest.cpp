/*
 * DefenceBehaviorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Analyzers/DangerHitMapAnalyzer.h"
#include "AI/Nullkiller2/Behaviors/DefenceBehavior.h"
#include "mock/TownFake.h"

#include "lib/mapObjects/CGHeroInstance.h"

namespace
{
NK2AI::HitMapInfo nextTurnThreat(uint64_t danger)
{
	NK2AI::HitMapInfo threat;
	threat.turn = 1;
	threat.danger = danger;
	return threat;
}
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, castleDefenceCountsCommittedDefenderAndTowers)
{
	test::TownFake town;
	town.withBuilding(BuildingID::CASTLE);

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 1));

	const auto townOnlyDefence = NK2AI::Goals::estimateTownDefence(*town.get(), nullptr);
	const auto committedDefence = NK2AI::Goals::estimateTownDefence(*town.get(), &defender);

	ASSERT_EQ(townOnlyDefence, 0);
	ASSERT_GT(committedDefence, defender.getTotalStrength());

	const auto threat = nextTurnThreat(committedDefence - 1);
	EXPECT_TRUE(NK2AI::Goals::shouldLockTownDefender(*town.get(), defender, threat, 1.0f))
		<< "a mobile hero must be locked when hero plus castle towers cover the threat";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, towerDefenceNeedsCommittedDefender)
{
	test::TownFake town;
	town.withBuilding(BuildingID::CASTLE);
	ASSERT_TRUE(town.get()->setCreature(SlotID(0), CreatureID::ARCHER, 1));

	const auto townArmyDefence = town.get()->getArmyStrength();
	EXPECT_EQ(NK2AI::Goals::estimateTownDefence(*town.get(), nullptr), townArmyDefence) << "uncommitted town defence should not rely on tower value";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, defenderLockIsSkippedWhenTownArmyAlreadyCoversThreat)
{
	test::TownFake town;
	ASSERT_TRUE(town.get()->setCreature(SlotID(0), CreatureID::ARCHER, 100));

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 1));

	const auto townOnlyDefence = NK2AI::Goals::estimateTownDefence(*town.get(), nullptr);
	ASSERT_GT(townOnlyDefence, 0);

	const auto threat = nextTurnThreat(townOnlyDefence);
	EXPECT_FALSE(NK2AI::Goals::shouldLockTownDefender(*town.get(), defender, threat, 1.0f)) << "do not reserve a mobile hero when town stacks already suffice";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, defenderLockIsOnlyForImmediateTownThreats)
{
	test::TownFake town;
	town.withBuilding(BuildingID::CASTLE);

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 1));

	NK2AI::HitMapInfo distantThreat = nextTurnThreat(NK2AI::Goals::estimateTownDefence(*town.get(), &defender) - 1);
	distantThreat.turn = 2;

	EXPECT_FALSE(NK2AI::Goals::shouldLockTownDefender(*town.get(), defender, distantThreat, 1.0f)) << "distant threats should stay in normal defence planning";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, requiredDefenderCoversAllUrgentTownThreats)
{
	test::TownFake town;
	town.withBuilding(BuildingID::CASTLE);

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 1));

	const auto committedDefence = NK2AI::Goals::estimateTownDefence(*town.get(), &defender);
	std::vector<NK2AI::HitMapInfo> threats = {
		nextTurnThreat(committedDefence - 1),
		nextTurnThreat(committedDefence)
	};

	EXPECT_TRUE(NK2AI::Goals::isHeroRequiredForTownDefence(*town.get(), defender, threats, 1.0f))
		<< "reserve the town hero when the same hero makes every urgent threat survivable";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, requiredDefenderCanBeNeededBeforeFullDefenceIsStable)
{
	test::TownFake town;
	town.withBuilding(BuildingID::CASTLE);

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 1));

	const auto committedDefence = NK2AI::Goals::estimateTownDefence(*town.get(), &defender);
	std::vector<NK2AI::HitMapInfo> threats = {
		nextTurnThreat(committedDefence - 1),
		nextTurnThreat(committedDefence + 1)
	};

	EXPECT_TRUE(NK2AI::Goals::isHeroRequiredForTownDefence(*town.get(), defender, threats, 1.0f))
		<< "reserve a defender that covers one urgent threat, even when another threat still needs a follow-up defence action";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, requiredDefenderIsSkippedWhenItCoversNoUrgentThreat)
{
	test::TownFake town;
	town.withBuilding(BuildingID::CASTLE);

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 1));

	const auto committedDefence = NK2AI::Goals::estimateTownDefence(*town.get(), &defender);
	std::vector<NK2AI::HitMapInfo> threats = {
		nextTurnThreat(committedDefence + 1)
	};

	EXPECT_FALSE(NK2AI::Goals::isHeroRequiredForTownDefence(*town.get(), defender, threats, 1.0f))
		<< "do not reserve a defender that cannot make any urgent town threat survivable";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, reserveDefenderThatImprovesUrgentTownDefence)
{
	test::TownFake town;
	town.withBuilding(BuildingID::CASTLE);

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 1));

	const auto committedDefence = NK2AI::Goals::estimateTownDefence(*town.get(), &defender);
	std::vector<NK2AI::HitMapInfo> threats = {
		nextTurnThreat(committedDefence + 1)
	};

	ASSERT_FALSE(NK2AI::Goals::isHeroRequiredForTownDefence(*town.get(), defender, threats, 1.0f))
		<< "this setup documents a defender that improves defence but does not fully cover the threat";

	EXPECT_TRUE(NK2AI::Goals::shouldReserveTownDefender(*town.get(), defender, threats, 1.0f))
		<< "do not treat a useful garrison defender as free while urgent town danger remains";
}
