/*
 * RebirthTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"


namespace
{

/// Counts divisible by five avoid random rounding
constexpr int32_t phoenixCount = 10;
constexpr int32_t rebornCount = 2;

constexpr int32_t slayerCount = 100;

}

class RebirthTest : public BattleTestFixture
{
public:
	/// Adds phoenixes and enough black dragons for a lethal attack
	void setUpBattle()
	{
		startGame();
		startBattle();

		phoenix = addStack(BattleSide::DEFENDER, creatureByName("core:phoenix"), BattleHex(rightHex), phoenixCount);
		slayer = addStack(BattleSide::ATTACKER, creatureByName("core:blackDragon"), BattleHex(leftHex), slayerCount);
	}

	/// Executes a lethal dragon attack against the phoenixes
	void slay()
	{
		ASSERT_TRUE(attack(slayer, BattleHex(rightHex)));
	}

	CStack * phoenix = nullptr;
	CStack * slayer = nullptr;
};

TEST_F(RebirthTest, ComesBackWithAFifthOfTheStackItStartedAs)
{
	setUpBattle();
	beginCombat();

	slay();

	EXPECT_TRUE(phoenix->alive()) << "the whole point of the ability is that the stack is not gone";
	EXPECT_EQ(phoenix->getCount(), rebornCount);
}

TEST_F(RebirthTest, ComesBackOnlyOnce)
{
	setUpBattle();
	beginCombat();

	slay();
	ASSERT_TRUE(phoenix->alive());

	slay();

	EXPECT_FALSE(phoenix->alive());
}

/// Verifies that rebirth does not resume the attack sequence
TEST_F(RebirthTest, TheKillingHitEndsItsAttack)
{
	setUpBattle();

	slayer->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::ADDITIONAL_ATTACK, BonusSource::OTHER, 1, BonusSourceID()));
	ASSERT_EQ(slayer->getTotalAttacks(false), 2);

	beginCombat();

	slay();

	EXPECT_TRUE(phoenix->alive()) << "the second attack must not target the resurrected stack";
	EXPECT_EQ(phoenix->getCount(), rebornCount);
}

/// Verifies that a resurrected stack cannot retaliate until its next turn
TEST_F(RebirthTest, AnswersNobodyDuringTheRoundItCameBackIn)
{
	setUpBattle();

	CStack * follower = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(rightHex + 2), 1);

	beginCombat();

	slay();
	ASSERT_TRUE(phoenix->alive());

	ASSERT_TRUE(attack(follower, BattleHex(rightHex + 1)));

	EXPECT_TRUE(phoenix->alive()) << "the follower is far too small to finish the reborn stack";
	EXPECT_EQ(follower->getCount(), 1) << "resurrected phoenixes must not retaliate";
}

TEST_F(RebirthTest, ACloneDoesNotComeBack)
{
	setUpBattle();
	beginCombat();

	// Set clone state without applying the Clone spell.
	makeClone(phoenix);

	slay();

	EXPECT_FALSE(phoenix->alive());
}

TEST_F(RebirthTest, ComesBackFromADeathBySpell)
{
	startGame();

	giveArtifact(attackerSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	attackerSideHero->addSpellToSpellbook(SpellID(SpellID::IMPLOSION));
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, 100, ChangeValueMode::ABSOLUTE);
	attackerSideHero->mana = 9999;

	startBattle();

	phoenix = addStack(BattleSide::DEFENDER, creatureByName("core:phoenix"), BattleHex(rightHex), phoenixCount);
	// Hero spell actions require an active allied unit.
	slayer = addStack(BattleSide::ATTACKER, creatureByName("core:blackDragon"), BattleHex(leftHex), slayerCount);

	beginCombat();

	ASSERT_TRUE(castAsHero(attackerSideHero, SpellID(SpellID::IMPLOSION), phoenix));

	EXPECT_TRUE(phoenix->alive());
	EXPECT_EQ(phoenix->getCount(), rebornCount);
}
