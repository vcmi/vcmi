/*
 * BallistaDamageTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/CCreatureHandler.h"

namespace
{

constexpr int32_t targetCount = 100;

struct BallistaCase
{
	const char * name;
	int heroAttack;
	int attackFromElsewhere;
	int multiplier;
};

}

class BallistaDamageTest : public BattleTestFixture
{
public:
	/// Base damage of one ballista
	static int baseDamage(bool maximum)
	{
		const auto * creature = CreatureID(CreatureID::BALLISTA).toEntity(LIBRARY);
		return maximum ? creature->getMaxDamage(false) : creature->getMinDamage(false);
	}

	/// Creates the attacking hero's ballista with optional excluded attack bonuses
	CStack * setUpBallista(int heroAttack, int attackFromElsewhere = 0)
	{
		startGame();

		attackerSideHero->setPrimarySkill(PrimarySkill::ATTACK, heroAttack, ChangeValueMode::ABSOLUTE);
		giveArtifact(attackerSideHero, ArtifactID(ArtifactID::BALLISTA), ArtifactPosition::MACH1);

		if(attackFromElsewhere != 0)
			attackerSideHero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::OTHER, attackFromElsewhere, BonusSourceID(), BonusSubtypeID(PrimarySkill::ATTACK)));

		startBattle();

		CStack * ballista = nullptr;
		for(const auto & unit : battle()->stacks)
			if(unit->creatureId() == CreatureID::BALLISTA)
				ballista = unit.get();

		EXPECT_NE(ballista, nullptr) << "ballista was not added";

		beginCombat();

		return ballista;
	}
};

class BallistaDamageScalingTest : public BallistaDamageTest, public ::testing::WithParamInterface<BallistaCase>
{
};

TEST_P(BallistaDamageScalingTest, ScalesByTheAttackOfItsHeroAlone)
{
	const CStack * ballista = setUpBallista(GetParam().heroAttack, GetParam().attackFromElsewhere);
	ASSERT_NE(ballista, nullptr);

	EXPECT_EQ(ballista->getMinDamage(true), baseDamage(false) * GetParam().multiplier);
	EXPECT_EQ(ballista->getMaxDamage(true), baseDamage(true) * GetParam().multiplier);
}

INSTANTIATE_TEST_SUITE_P(Scenarios, BallistaDamageScalingTest, ::testing::Values(
	BallistaCase{"everyAttackPointOfTheHeroCounts", 7, 0, 8},
	// Zero hero attack must preserve base damage.
	BallistaCase{"noAttackLeavesTheMachineAlone", 0, 0, 1},
	// Non-hero and non-artifact attack is excluded during battle setup.
	BallistaCase{"attackFromElsewhereIsIgnored", 0, 10, 1}
),
	[](const ::testing::TestParamInfo<BallistaCase> & info) { return info.param.name; });

/// Verifies that displayed and calculated damage use the same bonus-adjusted range
TEST_F(BallistaDamageTest, TheShownDamageIsTheDamageItDeals)
{
	constexpr int heroAttack = 5;

	CStack * ballista = setUpBallista(heroAttack);
	ASSERT_NE(ballista, nullptr);

	CStack * target = addStack(BattleSide::DEFENDER, creatureByName("core:hornedDemon"), BattleHex(rightHex), targetCount);

	// Equal attack and defence isolate the base damage range.
	const int defenceGap = ballista->getAttack(true) - target->getDefense(false);
	target->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::OTHER, defenceGap, BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE)));

	const auto estimate = battle()->calculateDmgRange(BattleAttackInfo(ballista, target, 0, true));

	EXPECT_EQ(estimate.damage.min, ballista->getMinDamage(true));
	EXPECT_EQ(estimate.damage.max, ballista->getMaxDamage(true));
}
