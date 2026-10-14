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
#include "AI/Nullkiller2/Behaviors/DefenceBehaviorUtils.h"
#include "AI/Nullkiller2/Pathfinding/AINodeStorage.h"
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

NK2AI::AIPath sameTurnPath(const CGHeroInstance & hero, float movementCost)
{
	NK2AI::AIPath path;
	path.targetHero = &hero;
	path.heroArmy = &hero;
	path.exchangeCount = 1;
	path.targetObjectDanger = 0;
	path.armyLoss = 0;
	path.targetObjectArmyLoss = 0;
	path.chainMask = 1;

	NK2AI::AIPathNodeInfo node;
	node.cost = movementCost;
	node.turns = 0;
	node.coord = int3(1, 1, 0);
	node.layer = EPathfindingLayer::LAND;
	node.danger = 0;
	node.targetHero = &hero;
	node.parentIndex = -1;
	node.chainMask = 1;
	node.actionIsBlocked = false;

	path.nodes.push_back(node);

	return path;
}

test::TownFake townTarget()
{
	test::TownFake town;
	town.get()->ID = Obj::TOWN;
	return town;
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

TEST(Nullkiller2_Behaviors_DefenceBehavior, reserveDefenderIsSkippedForDistantTownThreat)
{
	test::TownFake town;
	town.withBuilding(BuildingID::CASTLE);

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 1));

	const auto committedDefence = NK2AI::Goals::estimateTownDefence(*town.get(), &defender);
	auto distantThreat = nextTurnThreat(committedDefence + 1);
	distantThreat.turn = 2;

	EXPECT_FALSE(NK2AI::Goals::shouldReserveTownDefender(*town.get(), defender, { distantThreat }, 1.0f))
		<< "safe garrison heroes must stay available for extraction when there is no urgent town threat";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, sameTurnReturnPathAllowsSimpleRoundTrip)
{
	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 1));

	const auto path = sameTurnPath(defender, 0.45f);

	EXPECT_TRUE(NK2AI::Goals::isSafeSameTurnReturnPath(defender, path, 1.0f, 1.0f))
		<< "a garrison defender may raid only when the same turn has enough movement to return";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, defenderReleaseAllowsEnemyTownCaptureAfterArmyIsMostlyBought)
{
	auto targetTown = townTarget();

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 100));

	const auto remainingReinforcement = defender.getTotalStrength() / 20;
	EXPECT_TRUE(NK2AI::Goals::isDefenderReleaseAllowedForTownCapture(
		defender,
		*targetTown.get(),
		true,
		true,
		remainingReinforcement,
		3,
		7))
		<< "a defender can leave for an enemy town once the home town has no meaningful current-week army left to buy";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, defenderReleaseAllowsEnemyTownCaptureWhenHomeStillNeedsFollowup)
{
	auto targetTown = townTarget();

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 100));

	const auto remainingReinforcement = std::max<uint64_t>(1000, defender.getTotalStrength() / 20) + 1;
	EXPECT_TRUE(NK2AI::Goals::isDefenderReleaseAllowedForTownCapture(
		defender,
		*targetTown.get(),
		true,
		false,
		remainingReinforcement,
		3,
		7))
		<< "do not keep a defender idle when it cannot make the home town stable and can capture an enemy town instead";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, defenderReleaseRejectsNonEnemyTownCapture)
{
	auto targetTown = townTarget();

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 100));

	EXPECT_FALSE(NK2AI::Goals::isDefenderReleaseAllowedForTownCapture(
		defender,
		*targetTown.get(),
		false,
		false,
		0,
		3,
		7))
		<< "releasing a defender is reserved for enemy town captures, not neutral or allied visits";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, defenderReleaseRejectsMeaningfulUnboughtArmy)
{
	auto targetTown = townTarget();

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 100));

	const auto remainingReinforcement = std::max<uint64_t>(1000, defender.getTotalStrength() / 20) + 1;
	EXPECT_FALSE(NK2AI::Goals::isDefenderReleaseAllowedForTownCapture(
		defender,
		*targetTown.get(),
		true,
		true,
		remainingReinforcement,
		3,
		7))
		<< "buying the current-week army should happen before a defender takes a risky town-capture trip";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, defenderReleaseRejectsEndOfWeekGrowthRisk)
{
	auto targetTown = townTarget();

	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 100));

	EXPECT_FALSE(NK2AI::Goals::isDefenderReleaseAllowedForTownCapture(
		defender,
		*targetTown.get(),
		true,
		true,
		1,
		7,
		7))
		<< "do not risk losing fresh week growth unless the town is fully bought out";
}

TEST(Nullkiller2_Behaviors_DefenceBehavior, sameTurnReturnPathRejectsPathWithoutReturnMovement)
{
	CGHeroInstance defender(nullptr);
	ASSERT_TRUE(defender.setCreature(SlotID(0), CreatureID::ARCHER, 1));

	const auto path = sameTurnPath(defender, 0.6f);

	EXPECT_FALSE(NK2AI::Goals::isSafeSameTurnReturnPath(defender, path, 1.0f, 1.0f))
		<< "a one-way reachable attack is not enough when the defender must be back after turn end";
}
