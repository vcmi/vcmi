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

/// Every point of the hero's attack is worth another ballista's worth of damage.
TEST_F(BallistaDamageTest, DamageScalesWithTheAttackOfItsHero)
{
	constexpr int heroAttack = 7;

	const CStack * ballista = setUpBallista(heroAttack);
	ASSERT_NE(ballista, nullptr);

	EXPECT_EQ(ballista->getMinDamage(true), baseDamage(false) * (heroAttack + 1));
	EXPECT_EQ(ballista->getMaxDamage(true), baseDamage(true) * (heroAttack + 1));
}

/// A hero with no attack at all leaves the machine at what it deals on its own - and, since the
/// scaling is a bonus now rather than a step of the calculation, does not double it either.
TEST_F(BallistaDamageTest, AHeroWithoutAttackChangesNothing)
{
	const CStack * ballista = setUpBallista(0);
	ASSERT_NE(ballista, nullptr);

	EXPECT_EQ(ballista->getMinDamage(true), baseDamage(false));
	EXPECT_EQ(ballista->getMaxDamage(true), baseDamage(true));
}

/// Regression guard for the reason this was moved out of the client: what the windows show and what
/// the shot deals are now the same number, whatever the formula behind it is.
TEST_F(BallistaDamageTest, TheShownDamageIsTheDamageItDeals)
{
	constexpr int heroAttack = 5;

	CStack * ballista = setUpBallista(heroAttack);
	ASSERT_NE(ballista, nullptr);

	CStack * target = addStack(BattleSide::DEFENDER, creatureByName("core:hornedDemon"), BattleHex(rightHex), targetCount);
	ASSERT_NE(target, nullptr);

	// levelled up to the attack of the ballista, so that the attack-against-defense factor is 1 and
	// what is left of the estimate is the damage itself
	const int defenceGap = ballista->getAttack(true) - target->getDefense(false);
	target->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::OTHER, defenceGap, BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE)));

	const auto estimate = battle()->calculateDmgRange(BattleAttackInfo(ballista, target, 0, true));

	EXPECT_EQ(estimate.damage.min, ballista->getMinDamage(true));
	EXPECT_EQ(estimate.damage.max, ballista->getMaxDamage(true));
}

/// Only what the hero is worth on its own and what it wears counts. Attack from anywhere else -
/// an army-wide bonus, a spell, the terrain - does not reach the machine. The bonus is granted
/// before the battle is laid out, which is when the machine settles what it is worth.
TEST_F(BallistaDamageTest, OnlyTheAttackOfTheHeroItselfCounts)
{
	const CStack * ballista = setUpBallista(0, 10);
	ASSERT_NE(ballista, nullptr);

	EXPECT_EQ(ballista->getMinDamage(true), baseDamage(false)) << "the bonus comes from neither the hero itself nor an artifact";
	EXPECT_EQ(ballista->getMaxDamage(true), baseDamage(true)) << "the bonus comes from neither the hero itself nor an artifact";
}
