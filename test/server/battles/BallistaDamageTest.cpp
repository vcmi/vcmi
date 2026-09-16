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

/// One scenario: the attack of the hero owning the ballista, attack granted to that hero by
/// anything that is neither itself nor an artifact, and what the machine's own damage is multiplied
/// by as a result.
struct BallistaCase
{
	const char * name;
	int heroAttack;
	int attackFromElsewhere;
	int multiplier;
};

}

/// A ballista shoots for what the hero owning it is worth, and what it is worth is a bonus it is
/// handed when the battle is laid out. That is the whole of it: the number the windows show and the
/// number the damage calculator works from are the same one, because there is only one.
class BallistaDamageTest : public BattleTestFixture
{
public:
	/// Damage of one ballista before its hero is taken into account.
	static int baseDamage(bool maximum)
	{
		const auto * creature = CreatureID(CreatureID::BALLISTA).toEntity(LIBRARY);
		return maximum ? creature->getMaxDamage(false) : creature->getMinDamage(false);
	}

	/// Sets up a battle in which the attacking hero owns a ballista, and answers that ballista.
	/// `attackFromElsewhere` is attack granted to the hero by something that is neither itself nor
	/// an artifact, which the machine is not supposed to profit from.
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

		EXPECT_NE(ballista, nullptr) << "the war machine has to reach the battlefield";

		beginCombat();

		return ballista;
	}
};

/// Every point of the attack the hero is worth on its own or wears is another ballista's worth of
/// damage. Attack from anywhere else - an army-wide bonus, a spell, the terrain - does not reach
/// the machine, and a hero with no attack at all leaves it at what it deals on its own.
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
	// since the scaling is a bonus now rather than a step of the calculation, a hero without
	// attack must leave the damage alone rather than double or erase it
	BallistaCase{"noAttackLeavesTheMachineAlone", 0, 0, 1},
	// granted before the battle is laid out, which is when the machine settles what it is worth
	BallistaCase{"attackFromElsewhereIsIgnored", 0, 10, 1}
),
	[](const ::testing::TestParamInfo<BallistaCase> & info) { return info.param.name; });

/// Regression guard for the reason this was moved out of the client: what the windows show and what
/// the shot deals are now the same number, whatever the formula behind it is.
TEST_F(BallistaDamageTest, TheShownDamageIsTheDamageItDeals)
{
	constexpr int heroAttack = 5;

	CStack * ballista = setUpBallista(heroAttack);
	ASSERT_NE(ballista, nullptr);

	CStack * target = addStack(BattleSide::DEFENDER, creatureByName("core:hornedDemon"), BattleHex(rightHex), targetCount);

	// levelled up to the attack of the ballista, so that the attack-against-defense factor is 1 and
	// what is left of the estimate is the damage itself
	const int defenceGap = ballista->getAttack(true) - target->getDefense(false);
	target->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::OTHER, defenceGap, BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE)));

	const auto estimate = battle()->calculateDmgRange(BattleAttackInfo(ballista, target, 0, true));

	EXPECT_EQ(estimate.damage.min, ballista->getMinDamage(true));
	EXPECT_EQ(estimate.damage.max, ballista->getMaxDamage(true));
}
