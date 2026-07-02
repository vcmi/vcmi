/*
 * RecruitHeroBehaviorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Behaviors/RecruitHeroBehavior.h"
#include "lib/constants/NumericConstants.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CGTownInstance.h"

TEST(Nullkiller2_Behaviors_RecruitHeroBehavior, recruitDecisionAllowsMarkedNextTurnEmergencyUnderGoldPressure)
{
	CGHeroInstance tavernHero(nullptr);

	ASSERT_TRUE(tavernHero.setCreature(SlotID(0), CreatureID::ARCHER, 100));
	ASSERT_GT(tavernHero.getArmyCost(), GameConstants::HERO_GOLD_COST / 2.0);

	NK2AI::Goals::RecruitHeroChoice bestChoice;
	bestChoice.score = 1.0f;
	bestChoice.hero = &tavernHero;
	bestChoice.defensiveEmergency = true;

	EXPECT_TRUE(
		NK2AI::Goals::RecruitHeroBehavior::shouldRecruitHero(1, bestChoice, false, 0, 0, true)
	) << "severe next-turn town threat should bypass gold pressure for an emergency recruit";
}

TEST(Nullkiller2_Behaviors_RecruitHeroBehavior, defensiveEmergencyRequiresMeaningfulNextTurnThreat)
{
	CGTownInstance threatenedTown(nullptr);
	CGHeroInstance tavernHero(nullptr);

	ASSERT_TRUE(tavernHero.setCreature(SlotID(0), CreatureID::ARCHER, 100));

	NK2AI::HitMapInfo weakThreat;
	weakThreat.turn = 1;
	weakThreat.danger = GameConstants::HERO_GOLD_COST / 2 - 1;

	ASSERT_GT(tavernHero.getTotalStrength(), weakThreat.danger);
	EXPECT_FALSE(
		NK2AI::Goals::RecruitHeroBehavior::isDefensiveRecruitEmergency(threatenedTown, tavernHero, weakThreat, 1.0f)
	) << "weak next-turn threats should not force hero recruitment";
}

TEST(Nullkiller2_Behaviors_RecruitHeroBehavior, defensiveEmergencyAllowsSevereNextTurnThreatCoveredByHero)
{
	CGTownInstance threatenedTown(nullptr);
	CGHeroInstance tavernHero(nullptr);

	ASSERT_TRUE(tavernHero.setCreature(SlotID(0), CreatureID::ARCHER, 100));
	NK2AI::HitMapInfo threat;
	threat.turn = 1;
	threat.danger = GameConstants::HERO_GOLD_COST / 2;

	ASSERT_GT(tavernHero.getTotalStrength(), threat.danger);
	EXPECT_TRUE(
		NK2AI::Goals::RecruitHeroBehavior::isDefensiveRecruitEmergency(threatenedTown, tavernHero, threat, 1.0f)
	) << "severe next-turn threats should force recruitment when the tavern hero can cover them";
}
